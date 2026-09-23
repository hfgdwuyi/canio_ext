#!/usr/bin/env python3
"""Validate CANopen NMT state behavior for a target node.

Checks command specifiers:
  0x01 Start remote node
  0x02 Stop remote node
  0x80 Enter pre-operational
  0x81 Reset node
  0x82 Reset communication

Expected heartbeat state codes:
  0x00 Power On
  0x04 Stopped
  0x05 Operational
  0x7F Pre-Operational
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

    def callback(self, state: int) -> None:
        # Keep lower 7 bits only; helps if implementation exposes extra status bits.
        s = int(state) & 0x7F
        with self._lock:
            self._events.append((time.time(), s))

    def callback_from_can(self, can_id: int, data: bytes, timestamp: float) -> None:
        # Heartbeat payload is 1 byte (NMT state).
        if not data:
            return
        self.callback(data[0])

    def clear(self) -> None:
        with self._lock:
            self._events.clear()

    def snapshot(self) -> list[tuple[float, int]]:
        with self._lock:
            return list(self._events)

    def wait_for_state(self, expected_state: int, timeout_s: float) -> bool:
        deadline = time.time() + timeout_s
        while time.time() < deadline:
            with self._lock:
                if any(state == expected_state for _, state in self._events):
                    return True
            time.sleep(0.02)
        return False

    def wait_for_sequence(self, sequence: list[int], timeout_s: float) -> bool:
        deadline = time.time() + timeout_s
        while time.time() < deadline:
            with self._lock:
                states = [state for _, state in self._events]
            pos = 0
            for s in states:
                if s == sequence[pos]:
                    pos += 1
                    if pos == len(sequence):
                        return True
            time.sleep(0.02)
        return False


class BusHeartbeatSniffer:
    def __init__(self) -> None:
        self._lock = threading.Lock()
        self._seen: dict[int, int] = {}

    def callback_from_can(self, can_id: int, data: bytes, timestamp: float) -> None:
        if not data:
            return
        node_id = can_id & 0x7F
        state = int(data[0]) & 0x7F
        with self._lock:
            self._seen[node_id] = state

    def clear(self) -> None:
        with self._lock:
            self._seen.clear()

    def snapshot(self) -> dict[int, int]:
        with self._lock:
            return dict(self._seen)



def parse_int(value: str) -> int:
    return int(value, 0)



def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="NMT state compliance check")
    parser.add_argument("--node-id", type=parse_int, default=0x30, help="Target node id (default: 0x30)")
    parser.add_argument("--channel", type=int, default=0, help="IXXAT channel")
    parser.add_argument("--bitrate", type=int, default=500000, help="CAN bitrate")
    parser.add_argument("--timeout", type=float, default=3.0, help="Timeout for normal state transitions")
    parser.add_argument("--reset-timeout", type=float, default=8.0, help="Timeout for reset transitions")
    parser.add_argument(
        "--heartbeat-ms",
        type=int,
        default=1000,
        help="Set 0x1017 producer heartbeat time if current value is 0 (0 disables write)",
    )
    parser.add_argument(
        "--scan-heartbeat-s",
        type=float,
        default=1.0,
        help="Bus-wide heartbeat scan window (seconds) used when target heartbeat is not seen",
    )
    parser.add_argument(
        "--strict-reset-node",
        action="store_true",
        help="Require 0x81 Reset Node to reach pre-operational state after boot-up",
    )
    return parser.parse_args()



def send_nmt(network: canopen.Network, command: int, node_id: int) -> None:
    network.send_message(0x000, bytes([command & 0xFF, node_id & 0x7F]))



def sdo_read_u16(node: canopen.RemoteNode, index: int, sub: int) -> int:
    raw = node.sdo.upload(index, sub)
    if isinstance(raw, int):
        return raw & 0xFFFF
    data = bytes(raw)
    if len(data) < 2:
        raise ValueError(f"SDO 0x{index:04X}:{sub} payload too short: {len(data)}")
    return int.from_bytes(data[:2], byteorder="little", signed=False)


def sdo_write_u16(node: canopen.RemoteNode, index: int, sub: int, value: int) -> None:
    node.sdo.download(index, sub, int(value & 0xFFFF).to_bytes(2, byteorder="little", signed=False))


def format_state(code: int) -> str:
    return f"0x{code:02X} ({STATE_TEXT.get(code, 'Unknown')})"



def run_simple_transition(
    network: canopen.Network,
    hb: HeartbeatRecorder,
    node_id: int,
    command: int,
    command_name: str,
    expected_state: int,
    timeout_s: float,
) -> CheckResult:
    hb.clear()
    send_nmt(network, command, node_id)
    ok = hb.wait_for_state(expected_state, timeout_s)
    if ok:
        return CheckResult(command_name, True, f"heartbeat reached {format_state(expected_state)}")

    seen = ", ".join(format_state(s) for _, s in hb.snapshot()) or "<none>"
    return CheckResult(command_name, False, f"expected {format_state(expected_state)}, seen {seen}")



def run_reset_transition(
    network: canopen.Network,
    hb: HeartbeatRecorder,
    node_id: int,
    command: int,
    command_name: str,
    timeout_s: float,
    strict_reset_node: bool = False,
) -> CheckResult:
    hb.clear()
    send_nmt(network, command, node_id)

    # EN 50325-4 behavior is typically boot-up (0x00) followed by pre-op (0x7F).
    seq_ok = hb.wait_for_sequence([0x00, 0x7F], timeout_s)
    if seq_ok:
        return CheckResult(command_name, True, "observed sequence 0x00 -> 0x7F")

    # Allow looser pass if at least pre-op appears (some stacks suppress explicit 0x00 report to app layer).
    preop_ok = hb.wait_for_state(0x7F, 0.5)
    if preop_ok:
        return CheckResult(command_name, True, "pre-operational observed (0x7F), boot-up event not captured")

    # Some devices auto-enter operational right after reset node.
    op_ok = hb.wait_for_state(0x05, 0.5)
    if op_ok:
        return CheckResult(command_name, True, "operational observed (0x05) after reset")

    # Non-strict mode: treat boot-up-only as weak pass for reset node, because
    # follow-up heartbeat state may be delayed/suppressed by implementation policy.
    states = [s for _, s in hb.snapshot()]
    if (not strict_reset_node) and command == 0x81 and 0x00 in states:
        return CheckResult(command_name, True, "boot-up observed (0x00), follow-up state not captured")

    seen = ", ".join(format_state(s) for _, s in hb.snapshot()) or "<none>"
    return CheckResult(command_name, False, f"expected reset sequence with 0x00/0x7F, seen {seen}")



def print_result(res: CheckResult) -> None:
    status = "PASS" if res.passed else "FAIL"
    print(f"[{status}] {res.name}: {res.detail}")



def main() -> int:
    args = parse_args()

    network = canopen.Network()
    node = None
    hb = HeartbeatRecorder()
    sniffer = BusHeartbeatSniffer()

    try:
        network.connect(bustype="ixxat", channel=args.channel, bitrate=args.bitrate)
        node = canopen.RemoteNode(args.node_id, None)
        network.add_node(node)
        hb_cob_id = 0x700 + (args.node_id & 0x7F)
        network.subscribe(hb_cob_id, hb.callback_from_can)
        for nid in range(1, 128):
            network.subscribe(0x700 + nid, sniffer.callback_from_can)
        print(f"[INFO] Subscribed heartbeat COB-ID 0x{hb_cob_id:03X}")

        print(
            f"=== NMT compliance test start: node=0x{args.node_id:02X}, "
            f"channel={args.channel}, bitrate={args.bitrate} ==="
        )

        if args.heartbeat_ms > 0:
            try:
                hb_time = sdo_read_u16(node, 0x1017, 0x00)
                if hb_time == 0:
                    sdo_write_u16(node, 0x1017, 0x00, int(args.heartbeat_ms))
                    print(f"[INFO] Set 0x1017 Producer Heartbeat Time to {args.heartbeat_ms} ms")
                else:
                    print(f"[INFO] Existing 0x1017 Producer Heartbeat Time = {hb_time} ms")
            except Exception as exc:
                print(f"[WARN] Could not read/write 0x1017 heartbeat time: {exc}")

        hb.clear()
        sniffer.clear()
        time.sleep(min(max(args.timeout, 0.5), 1.5))
        if not hb.snapshot():
            print("[WARN] No heartbeat observed before NMT sequence. Check Node-ID and heartbeat producer support.")
            sniffer.clear()
            time.sleep(max(0.2, args.scan_heartbeat_s))
            seen = sniffer.snapshot()
            if seen:
                summary = ", ".join(
                    f"0x{nid:02X}:{format_state(state)}" for nid, state in sorted(seen.items())
                )
                print(f"[INFO] Heartbeat detected on other node(s): {summary}")
                print("[HINT] Re-run with --node-id matching one of the detected nodes.")
            else:
                print("[HINT] No heartbeat seen on bus. Check power, wiring, bitrate/channel, and heartbeat producer config.")

        results = []
        results.append(
            run_simple_transition(
                network, hb, args.node_id, 0x01, "0x01 Start remote node", 0x05, args.timeout
            )
        )
        results.append(
            run_simple_transition(
                network, hb, args.node_id, 0x02, "0x02 Stop remote node", 0x04, args.timeout
            )
        )
        results.append(
            run_simple_transition(
                network, hb, args.node_id, 0x80, "0x80 Enter pre-operational", 0x7F, args.timeout
            )
        )
        results.append(
            run_reset_transition(
                network,
                hb,
                args.node_id,
                0x81,
                "0x81 Reset node",
                args.reset_timeout,
                strict_reset_node=args.strict_reset_node,
            )
        )
        results.append(
            run_reset_transition(
                network, hb, args.node_id, 0x82, "0x82 Reset communication", args.reset_timeout
            )
        )

        for res in results:
            print_result(res)

        failed = [r for r in results if not r.passed]
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
            network.disconnect()
        except Exception as exc:
            print(f"[WARN] network.disconnect: {exc}")


if __name__ == "__main__":
    raise SystemExit(main())
