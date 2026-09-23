import argparse
import time

import can


TPDO1_DI_BITS = {
    1: {
        0: "CAN Power Supply Fault",
    },
    2: {
        6: "User Interface Input 7",
        5: "User Interface Input 6",
        4: "User Interface Input 5",
        3: "User Interface Input 4",
        2: "User Interface Input 3",
        1: "User Interface Input 2",
        0: "User Interface Input 1",
    },
    3: {
        6: "User Interface Input 7",
        5: "User Interface Input 6",
        4: "User Interface Input 5",
        3: "User Interface Input 4",
        2: "User Interface Input 3",
        1: "User Interface Input 2",
        0: "User Interface Input 1",
    },
    4: {
        1: "User Interface Voltage Detection",
        0: "I2C Temperature Sensor Alert",
    },
}


def parse_int(x) -> int:
    if isinstance(x, int):
        return x
    return int(str(x), 0)


def render_byte(byte_val: int) -> str:
    return f"0x{byte_val:02X} ({byte_val})"


def render_named_bits(sub: int, byte_val: int) -> list[str]:
    bit_map = TPDO1_DI_BITS.get(sub, {})
    lines = []
    for bit in sorted(bit_map.keys(), reverse=True):
        value = (byte_val >> bit) & 0x1
        lines.append(f"Bit{bit}: {bit_map[bit]} = {value}")
    return lines


def extract_named_bits_state(data: bytes) -> dict[tuple[int, int], int]:
    state = {}
    for sub in range(1, 5):
        byte_val = data[sub - 1] if len(data) >= sub else 0
        bit_map = TPDO1_DI_BITS.get(sub, {})
        for bit in bit_map:
            state[(sub, bit)] = (byte_val >> bit) & 0x1
    return state


def render_bit_changes(prev_data: bytes, curr_data: bytes) -> list[str]:
    prev_state = extract_named_bits_state(prev_data)
    curr_state = extract_named_bits_state(curr_data)
    lines = []

    for (sub, bit), curr_val in sorted(curr_state.items(), key=lambda x: (x[0][0], -x[0][1])):
        prev_val = prev_state.get((sub, bit), 0)
        if prev_val != curr_val:
            name = TPDO1_DI_BITS[sub][bit]
            lines.append(f"0x6000.{sub} unsigned 8 Bit{bit}: {name} {prev_val} -> {curr_val}")

    return lines


def print_change_details(cob_id: int, timestamp: float, data: bytes, prev_data: bytes, curr_data: bytes) -> int:
    changes = render_bit_changes(prev_data, curr_data)
    if not changes:
        return 0

    print(f"[TPDO1] cobid=0x{cob_id:03X} ts={timestamp:.3f} data=[{' '.join(f'{b:02X}' for b in data)}]")
    print("[CHANGES]")
    for line in changes:
        print(f"{line}")
    return len(changes)


def wait_for_tpdo1(bus, cob_id: int, timeout_s: float):
    if timeout_s <= 0:
        while True:
            msg = bus.recv(timeout=1.0)
            if msg is None:
                continue
            if getattr(msg, "is_extended_id", False):
                continue
            if int(getattr(msg, "arbitration_id", -1)) != cob_id:
                continue
            data = bytes(getattr(msg, "data", b""))
            ts = float(getattr(msg, "timestamp", time.time()))
            return ts, data

    deadline = time.time() + timeout_s
    while time.time() < deadline:
        remaining = max(0.0, deadline - time.time())
        msg = bus.recv(timeout=min(0.2, remaining))
        if msg is None:
            continue
        if getattr(msg, "is_extended_id", False):
            continue
        if int(getattr(msg, "arbitration_id", -1)) != cob_id:
            continue
        data = bytes(getattr(msg, "data", b""))
        ts = float(getattr(msg, "timestamp", time.time()))
        return ts, data
    return None, None


def resolve_channel(bustype: str, channel_value) -> object:
    if channel_value is not None:
        return channel_value
    return "PCAN_USBBUS1" if bustype == "pcan" else 0


def open_bus(bustype: str, channel_value, bitrate: int):
    channel = resolve_channel(bustype, channel_value)

    if bustype != "pcan":
        try:
            return can.interface.Bus(interface="ixxat", channel=channel, bitrate=bitrate)
        except TypeError:
            return can.interface.Bus(bustype="ixxat", channel=channel, bitrate=bitrate)

    if channel_value is not None:
        try:
            return can.interface.Bus(interface="pcan", channel=channel, bitrate=bitrate)
        except TypeError:
            return can.interface.Bus(bustype="pcan", channel=channel, bitrate=bitrate)

    candidates = []
    try:
        for cfg in can.detect_available_configs(interfaces=["pcan"]):
            ch = cfg.get("channel")
            if ch and ch not in candidates:
                candidates.append(ch)
    except Exception:
        pass

    for ch in ("PCAN_USBBUS1", "PCAN_USBBUS2", "PCAN_USBBUS3", "PCAN_USBBUS4"):
        if ch not in candidates:
            candidates.append(ch)

    last_exc = None
    for ch in candidates:
        try:
            bus = can.interface.Bus(interface="pcan", channel=ch, bitrate=bitrate)
            print(f"PCAN connected on channel: {ch}")
            return bus
        except TypeError:
            try:
                bus = can.interface.Bus(bustype="pcan", channel=ch, bitrate=bitrate)
                print(f"PCAN connected on channel: {ch}")
                return bus
            except Exception as exc:
                last_exc = exc
        except Exception as exc:
            last_exc = exc

    tried = ", ".join(str(c) for c in candidates) if candidates else "(none)"
    raise RuntimeError(
        "Failed to connect PCAN with auto channel detection. "
        f"Tried channels: {tried}. "
        "Please pass an explicit channel, e.g. --channel PCAN_USBBUS1"
    ) from last_exc


def build_arg_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(description="Realtime TPDO1 bit-change listener")
    ap.add_argument("bustype", nargs="?", choices=("ixxat", "pcan"), default=None, help="CAN backend to use: ixxat or pcan")
    ap.add_argument("--bustype", dest="bustype_opt", choices=("ixxat", "pcan"), default=None, help="CAN backend to use: ixxat or pcan")
    ap.add_argument("--count", type=int, default=0, help="0 means infinite")
    ap.add_argument("--timeout", type=float, default=0.0, help="0 means wait forever")
    ap.add_argument("--node-id", type=parse_int, default=0x30)
    ap.add_argument("--tpdo-cobid", type=parse_int, default=0x1B0)
    ap.add_argument("--channel", default=None, help="CAN interface channel (int for IXXAT, string for PCAN, e.g. PCAN_USBBUS1)")
    ap.add_argument("--bitrate", type=int, default=500000)
    return ap


def main() -> int:
    args = build_arg_parser().parse_args()
    args.bustype = args.bustype_opt if args.bustype_opt is not None else (args.bustype or "ixxat")

    bus = open_bus(args.bustype, args.channel, args.bitrate)

    try:
        received = 0
        prev_data = None
        while args.count == 0 or received < args.count:
            timestamp, data = wait_for_tpdo1(bus, args.tpdo_cobid, args.timeout)
            if data is None:
                if args.count == 0:
                    continue
                return 1

            if prev_data is None:
                prev_data = data
                received += 1
                continue

            _ = print_change_details(args.tpdo_cobid, timestamp, data, prev_data, data)

            prev_data = data
            received += 1

        return 0
    finally:
        try:
            bus.shutdown()
        except Exception:
            pass


if __name__ == "__main__":
    raise SystemExit(main())
