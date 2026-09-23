#!/usr/bin/env python3
from __future__ import annotations

import argparse
import time

import canopen
import can


def parse_int(x) -> int:
    if isinstance(x, int):
        return x
    return int(str(x), 0)


def read_u8(node, index: int, sub: int) -> int:
    raw = node.sdo.upload(index, sub)
    b = raw if isinstance(raw, (bytes, bytearray)) else bytes(raw)
    return b[0] if b else 0


def write_u8(node, index: int, sub: int, value: int) -> None:
    node.sdo.download(index, sub, bytes([int(value) & 0xFF]))


def read_u32(node, index: int, sub: int) -> int:
    raw = node.sdo.upload(index, sub)
    b = raw if isinstance(raw, (bytes, bytearray)) else bytes(raw)
    return int.from_bytes(b[:4], byteorder="little", signed=False) if b else 0


def write_u32(node, index: int, sub: int, value: int) -> None:
    node.sdo.download(index, sub, int(value).to_bytes(4, byteorder="little", signed=False))


def send_rpdo2(network, cob_id: int, b1: int, b2: int, b3: int, b4: int) -> None:
    network.send_message(cob_id, bytes([b1 & 0xFF, b2 & 0xFF, b3 & 0xFF, b4 & 0xFF]))


def read_5403_quartet(node) -> tuple[int, int, int, int]:
    return (
        read_u8(node, 0x5403, 1),
        read_u8(node, 0x5403, 2),
        read_u8(node, 0x5403, 3),
        read_u8(node, 0x5403, 4),
    )


def read_5401_quartet(node) -> tuple[int, int, int, int]:
    return (
        read_u8(node, 0x5401, 1),
        read_u8(node, 0x5401, 2),
        read_u8(node, 0x5401, 3),
        read_u8(node, 0x5401, 4),
    )


def parse_pattern(spec: str) -> tuple[int, int, int, int]:
    parts = [p.strip() for p in spec.split(",") if p.strip()]
    if len(parts) != 4:
        raise ValueError("pattern must be 4 bytes, e.g. 10,20,30,40")
    vals = tuple(parse_int(p) & 0xFF for p in parts)
    return vals


def parse_payload(spec: str) -> tuple[int, int, int, int]:
    return parse_pattern(spec)


def parse_enable_pattern(spec: str) -> tuple[int, int, int, int]:
    parts = [p.strip() for p in spec.split(",") if p.strip()]
    if len(parts) != 4:
        raise ValueError("enable pattern must be 4 values, e.g. 1,1,1,1")

    values = []
    for part in parts:
        if part.lower() in ("1", "true", "on", "enable", "enabled"):
            values.append(1)
        elif part.lower() in ("0", "false", "off", "disable", "disabled"):
            values.append(0)
        else:
            raise ValueError(f"invalid enable value: {part}")

    return tuple(values)


def build_single_channel_payload(node, sub: int, duty: int) -> tuple[int, int, int, int]:
    if sub < 1 or sub > 4:
        raise ValueError("sub must be 1..4 for 0x5403.1/.2/.3/.4")
    if duty < 0 or duty > 100:
        raise ValueError("duty must be in range 0..100")

    base = list(read_5403_quartet(node))
    base[sub - 1] = duty & 0xFF
    return tuple(base)


def validate_rpdo2_mapping(node) -> bool:
    try:
        map_count = read_u8(node, 0x1601, 0)
        mapped = []
        for sub in range(1, map_count + 1):
            entry = read_u32(node, 0x1601, sub)
            idx = (entry >> 16) & 0xFFFF
            subidx = (entry >> 8) & 0xFF
            bits = entry & 0xFF
            mapped.append((idx, subidx, bits))

        required = [(0x5403, 1, 0x08), (0x5403, 2, 0x08), (0x5403, 3, 0x08), (0x5403, 4, 0x08)]
        missing = [m for m in required if m not in mapped]
        if missing:
            print(f"[FAIL] RPDO2 map(0x1601) missing: {missing}; actual={mapped}")
            return False

        print(f"[PASS] RPDO2 map(0x1601) includes 0x5403.1/.2/.3/.4; actual={mapped}")
        return True
    except Exception as exc:
        print(f"[FAIL] RPDO2 mapping read failed: {exc}")
        return False


def resolve_rpdo2_cobid(node, override: int | None) -> int | None:
    if override is not None:
        cobid = int(override) & 0x7FF
        print(f"[INFO] RPDO2 COB-ID override = 0x{cobid:03X}")
        return cobid

    try:
        raw_cobid = read_u32(node, 0x1401, 1)
        if (raw_cobid & 0x80000000) != 0:
            print(f"[FAIL] RPDO2 invalid/disabled (0x1401:1=0x{raw_cobid:08X})")
            return None
        cobid = raw_cobid & 0x7FF
        print(f"[INFO] RPDO2 COB-ID = 0x{cobid:03X} (raw=0x{raw_cobid:08X})")
        return cobid
    except Exception as exc:
        print(f"[FAIL] RPDO2 COB-ID read failed: {exc}")
        return None


def apply_pwm_frequency(node, freq_hz: int, verify: bool = True) -> bool:
    if freq_hz <= 0:
        print("[FAIL] PWM frequency must be > 0")
        return False

    try:
        write_u32(node, 0x5402, 0, freq_hz)
        print(f"[INFO] Wrote 0x5402:0 = {freq_hz} Hz")
    except Exception as exc:
        print(f"[FAIL] Write 0x5402:0 failed: {exc}")
        return False

    if not verify:
        return True

    try:
        rb = read_u32(node, 0x5402, 0)
        ok = rb == int(freq_hz)
        print(f"[{'PASS' if ok else 'FAIL'}] Readback 0x5402:0 = {rb} Hz")
        return ok
    except Exception as exc:
        print(f"[FAIL] Readback 0x5402:0 failed: {exc}")
        return False


def apply_pwm_enable(node, states: tuple[int, int, int, int], verify: bool = True) -> bool:
    try:
        for sub, value in enumerate(states, start=1):
            write_u8(node, 0x5401, sub, value)
        print(f"[INFO] Wrote 0x5401:1..4 = {states}")
    except Exception as exc:
        print(f"[FAIL] Write 0x5401 failed: {exc}")
        return False

    if not verify:
        return True

    try:
        rb = read_5401_quartet(node)
        ok = rb == states
        print(f"[{'PASS' if ok else 'FAIL'}] Readback 0x5401:1..4 = {rb}")
        return ok
    except Exception as exc:
        print(f"[FAIL] Readback 0x5401 failed: {exc}")
        return False


def resolve_channel(bustype: str, channel_value) -> object:
    if channel_value is not None:
        return channel_value
    return "PCAN_USBBUS1" if bustype == "pcan" else 0


def build_connect_kwargs(bustype: str, channel_value, bitrate: int) -> dict:
    channel = resolve_channel(bustype, channel_value)
    if bustype == "pcan":
        return {"interface": "pcan", "channel": channel, "bitrate": bitrate}
    return {"bustype": "ixxat", "channel": channel, "bitrate": bitrate}


def connect_network(network: canopen.Network, bustype: str, channel_value, bitrate: int) -> None:
    if bustype != "pcan":
        connect_kwargs = build_connect_kwargs(bustype, channel_value, bitrate)
        try:
            network.connect(**connect_kwargs)
        except TypeError:
            network.connect(bustype="ixxat", channel=connect_kwargs["channel"], bitrate=bitrate)
        return

    if channel_value is not None:
        connect_kwargs = build_connect_kwargs("pcan", channel_value, bitrate)
        try:
            network.connect(**connect_kwargs)
            return
        except TypeError:
            network.connect(bustype="pcan", channel=connect_kwargs["channel"], bitrate=bitrate)
            return

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
            network.connect(interface="pcan", channel=ch, bitrate=bitrate)
            print(f"PCAN connected on channel: {ch}")
            return
        except TypeError:
            try:
                network.connect(bustype="pcan", channel=ch, bitrate=bitrate)
                print(f"PCAN connected on channel: {ch}")
                return
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
    ap = argparse.ArgumentParser(description="PWM test: 0x5401/0x5402 via SDO, RPDO2 only for 0x5403.1/.2/.3/.4")
    ap.add_argument("bustype", nargs="?", choices=("ixxat", "pcan"), default=None, help="CAN backend to use: ixxat or pcan")
    ap.add_argument("--bustype", dest="bustype_opt", choices=("ixxat", "pcan"), default=None, help="CAN backend to use: ixxat or pcan")
    ap.add_argument("--node-id", type=parse_int, default=0x30)
    ap.add_argument("--channel", default=None, help="CAN interface channel (int for IXXAT, string for PCAN, e.g. PCAN_USBBUS1)")
    ap.add_argument("--bitrate", type=int, default=500000)
    ap.add_argument("--rpdo2-cobid", type=parse_int, default=None, help="Optional COB-ID override")
    ap.add_argument("--enable", type=parse_enable_pattern, default=None, help="Optional 0x5401 enable pattern via SDO, e.g. 1,1,1,1")
    ap.add_argument("--no-enable-readback", dest="enable_readback", action="store_false", help="Skip 0x5401 readback after --enable")
    ap.add_argument("--freq", type=parse_int, default=None, help="Optional PWM carrier frequency in Hz (write 0x5402:0)")
    ap.add_argument("--no-freq-readback", dest="freq_readback", action="store_false", help="Skip 0x5402:0 readback after --freq")
    ap.add_argument("--settle-delay", type=float, default=0.08)
    ap.add_argument("--no-readback", dest="readback", action="store_false")

    ap.add_argument(
        "--pattern",
        action="append",
        default=[],
        help="One payload pattern, e.g. --pattern 10,20,30,40 (can repeat)",
    )
    ap.add_argument("--payload", help="Send one payload, e.g. --payload 10,20,30,40")

    ap.add_argument("sub", nargs="?", type=parse_int, default=None, help="RPDO2 channel subindex (1..4)")
    ap.add_argument("duty", nargs="?", type=parse_int, default=None, help="Duty value for selected channel (0..100)")

    ap.set_defaults(readback=True, enable_readback=True, freq_readback=True)
    return ap


def main() -> int:
    args = build_arg_parser().parse_args()
    args.bustype = args.bustype_opt if args.bustype_opt is not None else (args.bustype or "ixxat")

    default_patterns = [
        (0, 0, 0, 0),
        (10, 20, 30, 40),
        (40, 30, 20, 10),
        (100, 75, 50, 25),
    ]

    patterns = default_patterns

    network = canopen.Network()
    node = canopen.RemoteNode(args.node_id, None)
    network.add_node(node)

    try:
        connect_network(network, args.bustype, args.channel, args.bitrate)
        node.nmt.send_command(1)
        time.sleep(0.1)

        if not validate_rpdo2_mapping(node):
            return 1

        if args.enable is not None and not apply_pwm_enable(node, args.enable, args.enable_readback):
            return 1

        if args.freq is not None and not apply_pwm_frequency(node, int(args.freq), args.freq_readback):
            return 1

        rpdo2_cobid = resolve_rpdo2_cobid(node, args.rpdo2_cobid)
        if rpdo2_cobid is None:
            return 1

        # RPDO1-like usage: python test_rpdo2.py <sub> <duty>
        if args.sub is not None and args.duty is not None:
            patterns = [build_single_channel_payload(node, int(args.sub), int(args.duty))]
        elif args.pattern:
            patterns = [parse_pattern(p) for p in args.pattern]
        elif args.payload:
            patterns = [parse_payload(args.payload)]

        for p1, p2, p3, p4 in patterns:
            exp = (p1, p2, p3, p4)
            send_rpdo2(network, rpdo2_cobid, p1, p2, p3, p4)
            time.sleep(args.settle_delay)

            if args.readback:
                rb = read_5403_quartet(node)
                ok = rb == exp
                print(f"[{'PASS' if ok else 'FAIL'}] RPDO2 write={exp} read={rb}")
                if not ok:
                    return 1
            else:
                print(f"[INFO] RPDO2 write={exp}")

        print("=== SUMMARY: PASS ===")
        return 0
    finally:
        try:
            network.disconnect()
        except Exception as exc:
            print(f"[WARN] network.disconnect failed: {exc}")


if __name__ == "__main__":
    raise SystemExit(main())
