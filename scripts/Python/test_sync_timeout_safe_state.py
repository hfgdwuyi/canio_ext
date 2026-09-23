#!/usr/bin/env python3
"""Validate SYNC-loss handling and safe-state NMT behavior.

Test flow:
1) Enter OPERATIONAL and (optionally) configure heartbeat + SYNC timeout.
2) Send periodic SYNC frames for a stabilization window.
3) Stop SYNC frames and verify node falls back to PRE-OPERATIONAL.
4) Keep SYNC stopped and verify node stays in PRE-OPERATIONAL.
5) Re-enable SYNC, send NMT Start, and verify node returns to OPERATIONAL.

Notes:
- This script validates NMT-state transitions around SYNC loss.
- "Reject button input in safe state" is hardware/application-specific and is
  exposed here as a manual checklist printed at the end.
"""

from __future__ import annotations

import argparse
import threading
import time
from dataclasses import dataclass

import canopen


STATE_TEXT = {
    0x00: "Power On",
    0x04: "Stopped",
    0x05: "Operational",
    0x7F: "Pre-Operational",
}


@dataclass
class CheckResult:
    name: str
    passed: bool
    detail: str


class HeartbeatRecorder:
    def __init__(self) -> None:
        self._lock = threading.Lock()
        self._events: list[tuple[float, int]] = []

    def callback_from_can(self, can_id: int, data: bytes, timestamp: float) -> None:
        if not data:
            return
        state = int(data[0]) & 0x7F
        with self._lock:
            self._events.append((time.time(), state))

    def clear(self) -> None:
        with self._lock:
            self._events.clear()

    def snapshot(self) -> list[tuple[float, int]]:
        with self._lock:
            return list(self._events)

    def last_state(self) -> int | None:
        with self._lock:
            if not self._events:
                return None
            return self._events[-1][1]

    def wait_for_state(self, expected_state: int, timeout_s: float) -> bool:
        deadline = time.time() + timeout_s
        while time.time() < deadline:
            with self._lock:
                if any(state == expected_state for _, state in self._events):
                    return True
            time.sleep(0.02)
        return False


class SyncProducer:
    def __init__(self, network: canopen.Network, cob_id: int, period_ms: float) -> None:
        self._network = network
        self._cob_id = cob_id
        self._period_s = max(0.001, period_ms / 1000.0)
        self._stop_event = threading.Event()
        self._thread: threading.Thread | None = None

    def start(self) -> None:
        if self._thread and self._thread.is_alive():
            return
        self._stop_event.clear()
        self._thread = threading.Thread(target=self._run, name="sync-producer", daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._stop_event.set()
        if self._thread:
            self._thread.join(timeout=1.0)

    def _run(self) -> None:
        while not self._stop_event.is_set():
            self._network.send_message(self._cob_id, b"")
            time.sleep(self._period_s)


def parse_int(value: str) -> int:
    return int(value, 0)


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="SYNC-timeout / safe-state validation")
    p.add_argument("--node-id", type=parse_int, default=0x30, help="Target node id (default: 0x30)")
    p.add_argument("--channel", type=int, default=0, help="IXXAT channel")
    p.add_argument("--bitrate", type=int, default=500000, help="CAN bitrate")
    p.add_argument("--heartbeat-ms", type=int, default=2000, help="Write 0x1017 if current value is 0 (0 disables write)")

    p.add_argument("--sync-cobid", type=parse_int, default=0x80, help="SYNC COB-ID (default: 0x80)")
    p.add_argument("--sync-period-ms", type=float, default=100.0, help="SYNC period in milliseconds")
    p.add_argument("--sync-stabilize-s", type=float, default=1.5, help="How long to keep SYNC running before stop test")

    p.add_argument("--nmt-timeout", type=float, default=3.0, help="Timeout for NMT state transitions")
    p.add_argument("--timeout-detect-s", type=float, default=6.0, help="Wait time after SYNC stop to detect PRE-OP")
    p.add_argument("--hold-check-s", type=float, default=1.5, help="Additional SYNC-missing hold check window")

    p.add_argument("--sync-timeout-index", type=parse_int, default=None, help="Optional OD index for programmable SYNC timeout")
    p.add_argument("--sync-timeout-sub", type=parse_int, default=0x00, help="Optional OD sub-index for programmable SYNC timeout")
    p.add_argument("--sync-timeout-ms", type=int, default=None, help="Optional SYNC timeout value to write")
    p.add_argument(
        "--sync-timeout-type",
        choices=("u16", "u32"),
        default="u16",
        help="Data width when writing --sync-timeout-ms",
    )

    return p.parse_args()


def format_state(code: int) -> str:
    return f"0x{code:02X} ({STATE_TEXT.get(code, 'Unknown')})"


def send_nmt(network: canopen.Network, command: int, node_id: int) -> None:
    network.send_message(0x000, bytes([command & 0xFF, node_id & 0x7F]))


def sdo_read_u16(node: canopen.RemoteNode, index: int, sub: int) -> int:
    raw = node.sdo.upload(index, sub)
    data = bytes(raw)
    if len(data) < 2:
        raise ValueError(f"SDO 0x{index:04X}:{sub} payload too short: {len(data)}")
    return int.from_bytes(data[:2], byteorder="little", signed=False)


def sdo_write_u16(node: canopen.RemoteNode, index: int, sub: int, value: int) -> None:
    node.sdo.download(index, sub, int(value & 0xFFFF).to_bytes(2, byteorder="little", signed=False))


def sdo_write_u32(node: canopen.RemoteNode, index: int, sub: int, value: int) -> None:
    node.sdo.download(index, sub, int(value & 0xFFFFFFFF).to_bytes(4, byteorder="little", signed=False))


def check_enter_operational(network: canopen.Network, hb: HeartbeatRecorder, node_id: int, timeout_s: float) -> CheckResult:
    hb.clear()
    send_nmt(network, 0x01, node_id)
    ok = hb.wait_for_state(0x05, timeout_s)
    if ok:
        return CheckResult("Enter OPERATIONAL", True, "heartbeat reached 0x05")
    seen = ", ".join(format_state(s) for _, s in hb.snapshot()) or "<none>"
    return CheckResult("Enter OPERATIONAL", False, f"expected 0x05, seen {seen}")


def check_timeout_to_preop(hb: HeartbeatRecorder, timeout_s: float) -> CheckResult:
    hb.clear()
    ok = hb.wait_for_state(0x7F, timeout_s)
    if ok:
        return CheckResult("SYNC loss -> PRE-OP", True, "heartbeat reached 0x7F")
    seen = ", ".join(format_state(s) for _, s in hb.snapshot()) or "<none>"
    return CheckResult("SYNC loss -> PRE-OP", False, f"expected 0x7F within timeout, seen {seen}")


def check_hold_preop(hb: HeartbeatRecorder, hold_s: float) -> CheckResult:
    time.sleep(max(0.1, hold_s))
    last = hb.last_state()
    if last == 0x7F:
        return CheckResult("Safe-state hold", True, f"state stayed {format_state(last)}")
    if last is None:
        return CheckResult("Safe-state hold", False, "no heartbeat observed during hold window")
    return CheckResult("Safe-state hold", False, f"state moved to {format_state(last)}")


def print_result(r: CheckResult) -> None:
    print(f"[{'PASS' if r.passed else 'FAIL'}] {r.name}: {r.detail}")


def main() -> int:
    args = parse_args()

    network = canopen.Network()
    node = None
    hb = HeartbeatRecorder()
    sync_prod = None

    try:
        network.connect(bustype="ixxat", channel=args.channel, bitrate=args.bitrate)
        node = canopen.RemoteNode(args.node_id, None)
        network.add_node(node)

        hb_cob_id = 0x700 + (args.node_id & 0x7F)
        network.subscribe(hb_cob_id, hb.callback_from_can)
        print(f"[INFO] Subscribed heartbeat COB-ID 0x{hb_cob_id:03X}")

        node.sdo.RESPONSE_TIMEOUT = 2.0
        node.sdo.MAX_RETRIES = 3

        if args.heartbeat_ms > 0:
            try:
                hb_time = sdo_read_u16(node, 0x1017, 0x00)
                if hb_time == 0:
                    sdo_write_u16(node, 0x1017, 0x00, int(args.heartbeat_ms))
                    print(f"[INFO] Set 0x1017 heartbeat producer time to {args.heartbeat_ms} ms")
                else:
                    print(f"[INFO] Existing 0x1017 heartbeat producer time = {hb_time} ms")
            except Exception as exc:
                print(f"[WARN] Could not read/write 0x1017: {exc}")

        if args.sync_timeout_index is not None and args.sync_timeout_ms is not None:
            try:
                if args.sync_timeout_type == "u16":
                    sdo_write_u16(node, args.sync_timeout_index, args.sync_timeout_sub, args.sync_timeout_ms)
                else:
                    sdo_write_u32(node, args.sync_timeout_index, args.sync_timeout_sub, args.sync_timeout_ms)
                print(
                    f"[INFO] Wrote SYNC timeout {args.sync_timeout_ms} ms to "
                    f"0x{args.sync_timeout_index:04X}:{args.sync_timeout_sub} ({args.sync_timeout_type})"
                )
            except Exception as exc:
                print(
                    f"[WARN] Could not write SYNC timeout to "
                    f"0x{args.sync_timeout_index:04X}:{args.sync_timeout_sub}: {exc}"
                )

        print(
            f"=== SYNC timeout test start: node=0x{args.node_id:02X}, "
            f"sync_cobid=0x{args.sync_cobid:03X}, sync_period={args.sync_period_ms}ms ==="
        )

        results: list[CheckResult] = []

        r_enter = check_enter_operational(network, hb, args.node_id, args.nmt_timeout)
        results.append(r_enter)
        print_result(r_enter)
        if not r_enter.passed:
            print("=== SUMMARY: FAIL (cannot enter OPERATIONAL) ===")
            return 1

        sync_prod = SyncProducer(network, args.sync_cobid, args.sync_period_ms)
        sync_prod.start()
        print(f"[INFO] Sending SYNC for {args.sync_stabilize_s:.2f}s")
        time.sleep(max(0.2, args.sync_stabilize_s))

        sync_prod.stop()
        print("[INFO] SYNC stopped, waiting for timeout reaction")

        r_timeout = check_timeout_to_preop(hb, args.timeout_detect_s)
        results.append(r_timeout)
        print_result(r_timeout)

        r_hold = check_hold_preop(hb, args.hold_check_s)
        results.append(r_hold)
        print_result(r_hold)

        sync_prod = SyncProducer(network, args.sync_cobid, args.sync_period_ms)
        sync_prod.start()
        print("[INFO] SYNC resumed, requesting OPERATIONAL to leave safe state")

        r_recover = check_enter_operational(network, hb, args.node_id, args.nmt_timeout)
        r_recover.name = "Leave safe state by OPERATIONAL"
        results.append(r_recover)
        print_result(r_recover)

        failed = [r for r in results if not r.passed]

        print("\n[MANUAL] Button-input safe-state checklist:")
        print("  1) In OPERATIONAL, verify button action has effect (baseline).")
        print("  2) Stop SYNC until node is PRE-OP (safe state).")
        print("  3) Verify button action is rejected in safe state.")
        print("  4) Send NMT Start and verify button action works again.")

        if failed:
            print(f"=== SUMMARY: FAIL ({len(failed)}/{len(results)} failed) ===")
            return 1

        print(f"=== SUMMARY: PASS ({len(results)}/{len(results)}) ===")
        return 0

    except Exception as exc:
        print(f"[FATAL] Test aborted: {exc}")
        return 2

    finally:
        try:
            if sync_prod is not None:
                sync_prod.stop()
        except Exception:
            pass
        try:
            network.disconnect()
        except Exception as exc:
            print(f"[WARN] network.disconnect: {exc}")


if __name__ == "__main__":
    raise SystemExit(main())
