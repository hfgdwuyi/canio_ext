
import argparse
import time
import threading

import canopen


STATE_TEXT = {
    0x00: "Power On",
    0x04: "Stopped",
    0x05: "Operational",
    0x7F: "Pre-Operational",
}


def resolve_channel(bustype: str, channel_value) -> object:
    if channel_value is not None:
        return channel_value
    return "PCAN_USBBUS1" if bustype == "pcan" else 0


def build_connect_kwargs(bustype: str, channel_value, bitrate: int) -> dict:
    channel = resolve_channel(bustype, channel_value)
    if bustype == "pcan":
        return {"interface": "pcan", "channel": channel, "bitrate": bitrate}
    return {"bustype": "ixxat", "channel": channel, "bitrate": bitrate}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Set node to OPERATIONAL and verify SDO response")
    parser.add_argument("bustype", nargs="?", choices=("ixxat", "pcan"), default=None, help="CAN backend to use: ixxat or pcan")
    parser.add_argument("--bustype", dest="bustype_opt", choices=("ixxat", "pcan"), default=None, help="CAN backend to use: ixxat or pcan")
    parser.add_argument("--node-id", type=lambda x: int(x, 0), default=0x34, help="CANopen node id")
    parser.add_argument("--channel", default=None, help="CAN interface channel (int for IXXAT, string for PCAN, e.g. PCAN_USBBUS1)")
    parser.add_argument("--bitrate", type=int, default=500000, help="CAN bitrate")
    parser.add_argument(
        "--scan-bitrate",
        action="store_true",
        help="Try common bitrates (1M/500k/250k/125k) until SDO read succeeds",
    )
    parser.add_argument(
        "--keep-sync",
        action="store_true",
        help="Send periodic SYNC while script is running to keep node in OP when SYNC watchdog is active",
    )
    parser.add_argument("--sync-cobid", type=lambda x: int(x, 0), default=0x80, help="SYNC COB-ID")
    parser.add_argument("--sync-period-ms", type=float, default=100.0, help="SYNC period in milliseconds")
    parser.add_argument(
        "--sync-runtime-s",
        type=float,
        default=10.0,
        help="How long to keep sending SYNC before disconnect (only with --keep-sync)",
    )
    args = parser.parse_args()
    args.bustype = args.bustype_opt if args.bustype_opt is not None else (args.bustype or "ixxat")
    return args


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


class HeartbeatRecorder:
    def __init__(self) -> None:
        self._lock = threading.Lock()
        self._events = []

    def callback_from_can(self, can_id: int, data: bytes, timestamp: float) -> None:
        if not data:
            return
        state = int(data[0]) & 0x7F
        with self._lock:
            self._events.append((time.time(), state))

    def snapshot(self):
        with self._lock:
            return list(self._events)

    def clear(self) -> None:
        with self._lock:
            self._events.clear()

    def last_state(self):
        with self._lock:
            if not self._events:
                return None
            return self._events[-1][1]


def format_state(code: int) -> str:
    return f"0x{code:02X} ({STATE_TEXT.get(code, 'Unknown')})"


def try_operational(
    node_id: int,
    channel: int,
    bitrate: int,
    bustype: str,
    keep_sync: bool,
    sync_cobid: int,
    sync_period_ms: float,
    sync_runtime_s: float,
) -> bool:
    network = canopen.Network()
    sync_prod = None
    hb = HeartbeatRecorder()

    connect_kwargs = build_connect_kwargs(bustype, channel, bitrate)
    try:
        network.connect(**connect_kwargs)
    except TypeError:
        # Compatibility fallback for python-can releases that use the older bustype keyword.
        if bustype == "pcan":
            connect_kwargs = {"bustype": "pcan", "channel": connect_kwargs["channel"], "bitrate": bitrate}
        else:
            connect_kwargs = {"bustype": "ixxat", "channel": connect_kwargs["channel"], "bitrate": bitrate}
        try:
            network.connect(**connect_kwargs)
        except Exception as exc:
            print(f"CAN bus connect failed (bitrate={bitrate}): {exc}")
            return False
    except Exception as exc:
        print(f"CAN bus connect failed (bitrate={bitrate}): {exc}")
        return False

    try:
        node = canopen.RemoteNode(node_id, None)
        network.add_node(node)
        hb_cobid = 0x700 + (node_id & 0x7F)
        network.subscribe(hb_cobid, hb.callback_from_can)

        node.sdo.RESPONSE_TIMEOUT = 2.0
        node.sdo.MAX_RETRIES = 3

        try:
            sync_cycle_us = int.from_bytes(node.sdo.upload(0x1006, 0x00), byteorder="little", signed=False)
        except Exception:
            sync_cycle_us = None

        try:
            hb_ms = int.from_bytes(node.sdo.upload(0x1017, 0x00), byteorder="little", signed=False)
        except Exception:
            hb_ms = None

        if sync_cycle_us and sync_cycle_us > 0 and not keep_sync:
            print(
                "[WARN] 0x1006 (SYNC cycle period) is non-zero. "
                "Without periodic SYNC, node can fall back to PRE-OP and CAN-RUN will blink."
            )

        if keep_sync:
            sync_prod = SyncProducer(network, sync_cobid, sync_period_ms)
            sync_prod.start()
            print(
                f"Started SYNC producer: cobid=0x{sync_cobid:03X}, period={sync_period_ms}ms, "
                f"runtime={sync_runtime_s}s"
            )

        node.nmt.state = 'OPERATIONAL'
        print(f"Sent NMT Operational to node 0x{node_id:02X} (bitrate={bitrate})")
        time.sleep(0.1)

        try:
            raw = node.sdo.upload(0x100A, 0x00)
            sw_version = raw.decode("ascii", errors="ignore").rstrip("\x00")
            print(f"Node 0x{node_id:02X} SDO responded: 0x100A='{sw_version}'")

            if hb_ms is None:
                print("Note: Could not read 0x1017 heartbeat producer time.")
            elif hb_ms == 0:
                print("Note: 0x1017=0, producer heartbeat is disabled, so heartbeat cannot confirm NMT state.")
            else:
                print(f"Note: 0x1017={hb_ms} ms, heartbeat is enabled.")

            if sync_cycle_us is not None:
                print(f"Note: 0x1006={sync_cycle_us} us.")

            if keep_sync:
                time.sleep(max(0.1, sync_runtime_s))
                st = hb.last_state()
                if st is not None:
                    print(f"Heartbeat state while SYNC active: {format_state(st)}")
            else:
                if hb_ms and hb_ms > 0:
                    hb.clear()
                    # Observe for > heartbeat period and > SYNC watchdog timeout to show fallback.
                    time.sleep(max(2.2, (hb_ms / 1000.0) + 0.3, (sync_cycle_us or 0) / 1_000_000.0 + 0.5))
                    states = [s for _, s in hb.snapshot()]
                    if states:
                        print("Observed heartbeat states after NMT:", ", ".join(format_state(s) for s in states[-6:]))
                        print(f"Final observed state: {format_state(states[-1])}")
                    else:
                        print("No heartbeat observed during post-NMT observation window.")

            return True
        except Exception as exc:
            print(f"Node 0x{node_id:02X} has no SDO response; CANopen communication cannot be confirmed: {exc}")
            msg = str(exc)
            if (
                "Error warning limit exceeded" in msg
                or "data overrun" in msg.lower()
                or "bit stuff" in msg.lower()
                or "form error" in msg.lower()
            ):
                print("\n[DIAG] CAN physical layer or bus quality issue detected, not just an SDO timeout.")
                print("[DIAG] Check these items first:")
                print("  1) Verify bitrate consistency (script=500k; device/FDCAN/other nodes must match)")
                print("  2) Verify termination resistors (120 ohm at both bus ends)")
                print("  3) Verify CAN_H/CAN_L wiring and ground reference")
                print("  4) Check if the bus is saturated by other high-load processes causing IXXAT RX overrun")
                print("  5) Check if a node is flooding error frames (persistent bit stuff / form error)")
            return False

        # Alternative: use send_command style (broadcast to all nodes):
        # network.nmt.send_command(0x01)  # 0x01 = Start all nodes

    finally:
        if sync_prod is not None:
            try:
                sync_prod.stop()
            except Exception as exc:
                print(f"sync producer stop warning (ignored): {exc}")
        try:
            network.disconnect()
        except Exception as exc:
            print(f"network.disconnect warning (ignored): {exc}")


def main():
    args = parse_args()

    if not args.scan_bitrate:
        _ = try_operational(
            args.node_id,
            args.channel,
            args.bitrate,
            args.bustype,
            args.keep_sync,
            args.sync_cobid,
            args.sync_period_ms,
            args.sync_runtime_s,
        )
        return

    print("Starting automatic bitrate scan: 1000000, 500000, 250000, 125000")
    for br in (1000000, 500000, 250000, 125000):
        ok = try_operational(
            args.node_id,
            args.channel,
            br,
            args.bustype,
            args.keep_sync,
            args.sync_cobid,
            args.sync_period_ms,
            args.sync_runtime_s,
        )
        if ok:
            print(f"[OK] Reachable bitrate: {br}")
            return
        print(f"[FAIL] Communication failed at bitrate {br}")

    print("All candidate bitrates failed. Prioritize physical-layer checks (termination/wiring/ground/faulty node).")


if __name__ == "__main__":
    main()
