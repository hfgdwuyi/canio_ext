import argparse
import time

import canopen
import can


RPDO1_BIT_MAP = {
    1: {
        4: "UI Output L Pin9",
        3: "UI Output L Pin8",
        2: "UI Output L Pin7",
        1: "UI Output L Pin6",
        0: "UI Output L Pin5",
    },
    2: {
        4: "UI Output R Pin9",
        3: "UI Output R Pin8",
        2: "UI Output R Pin7",
        1: "UI Output R Pin6",
        0: "UI Output R Pin5",
    },
    3: {
        6: "CAN Power Supply Enable",
        5: "User Interface Power Supply Enable",
        4: "User Interface LED output Enable",
        3: "E-Break-4 power enable",
        2: "E-Break-3 power enable",
        1: "E-Break-2 power enable",
        0: "E-Break-1 power enable",
    },
}


def print_sub_map(sub: int) -> None:
    print(f"0x6200.{sub} unsigned 8")
    for bit in sorted(RPDO1_BIT_MAP[sub].keys(), reverse=True):
        print(f"Bit{bit}: {RPDO1_BIT_MAP[sub][bit]}")


def print_control_target(sub: int, bit: int) -> None:
    label = RPDO1_BIT_MAP.get(sub, {}).get(bit, "Unknown control")
    print(f"[INFO] Control target: 0x6200.{sub} Bit{bit} -> {label}")


def parse_int(x) -> int:
    if isinstance(x, int):
        return x
    return int(str(x), 0)


def parse_payload(spec: str) -> tuple[int, int, int]:
    parts = [p.strip() for p in spec.split(",") if p.strip()]
    if len(parts) != 3:
        raise ValueError("--payload must be 3 comma-separated bytes, e.g. 0x01,0x00,0x40")
    vals = tuple(parse_int(p) & 0xFF for p in parts)
    return vals


def read_u8(node, index: int, sub: int) -> int:
    raw = node.sdo.upload(index, sub)
    b = raw if isinstance(raw, (bytes, bytearray)) else bytes(raw)
    return b[0] if b else 0


def send_rpdo1(network, cob_id: int, b1: int, b2: int, b3: int) -> None:
    network.send_message(cob_id, bytes([b1 & 0xFF, b2 & 0xFF, b3 & 0xFF]))


def read_6200_triplet(node) -> tuple[int, int, int]:
    return (
        read_u8(node, 0x6200, 1),
        read_u8(node, 0x6200, 2),
        read_u8(node, 0x6200, 3),
    )


def ask_continue() -> bool:
    ans = input("Is the signal level correct? [y=match, n=mismatch, q=quit]: ").strip().lower()
    return ans != "q"


def run_payload_mode(network, node, args: argparse.Namespace) -> int:
    b1, b2, b3 = parse_payload(args.payload)
    send_rpdo1(network, args.rpdo_cobid, b1, b2, b3)
    time.sleep(args.settle_delay)

    if args.readback:
        r1, r2, r3 = read_6200_triplet(node)
        ok = (r1, r2, r3) == (b1, b2, b3)
        tag = "PASS" if ok else "FAIL"
        print(f"[{tag}] write=[0x{b1:02X},0x{b2:02X},0x{b3:02X}] read=[0x{r1:02X},0x{r2:02X},0x{r3:02X}]")
        return 0 if ok else 1

    return 0


def run_pulse_mode(network, node, args: argparse.Namespace) -> int:
    if args.sub not in (1, 2, 3):
        raise ValueError("--sub must be 1/2/3 for RPDO1 mapped 0x6200 bytes")
    if args.bit < 0 or args.bit > 7:
        raise ValueError("--bit must be in range 0..7")

    base = list(read_6200_triplet(node))
    print_sub_map(args.sub)
    print_control_target(args.sub, args.bit)

    idx = args.sub - 1
    mask = 1 << args.bit
    sequence = (0, 1, 0)

    for target in sequence:
        tx = base.copy()
        tx[idx] = (tx[idx] & (~mask & 0xFF)) | (target << args.bit)
        print(f"[TX] sub={args.sub} bit={args.bit} write={target}")
        send_rpdo1(network, args.rpdo_cobid, tx[0], tx[1], tx[2])
        time.sleep(args.settle_delay)

        if args.readback:
            rb = list(read_6200_triplet(node))
            rb_bit = (rb[idx] >> args.bit) & 0x1
            ok = rb_bit == target
            tag = "PASS" if ok else "FAIL"
            print(f"[{tag}] sub={args.sub} bit={args.bit} read={rb_bit} expect={target}")
            if not ok:
                return 1

        if args.manual_confirm and not ask_continue():
            print("[INFO] Aborted by user")
            return 2

    if args.restore:
        send_rpdo1(network, args.rpdo_cobid, base[0], base[1], base[2])
        time.sleep(args.settle_delay)

    return 0


def run_all_bits_mode(network, node, args: argparse.Namespace) -> int:
    print("[INFO] Sweep all mapped bits in RPDO1: 0x6200.1/.2/.3")
    for sub in (1, 2, 3):
        for bit in sorted(RPDO1_BIT_MAP[sub].keys(), reverse=True):
            label = RPDO1_BIT_MAP[sub][bit]
            print(f"--- test sub={sub} bit={bit} ({label}) ---")

            # Reuse existing pulse flow bit-by-bit to keep behavior identical.
            sub_args = argparse.Namespace(**vars(args))
            sub_args.sub = sub
            sub_args.bit = bit

            ret = run_pulse_mode(network, node, sub_args)
            if ret != 0:
                return ret

    return 0


def run_set_mode(network, node, args: argparse.Namespace) -> int:
    if args.sub not in (1, 2, 3):
        raise ValueError("sub must be 1/2/3 for RPDO1 mapped 0x6200 bytes")
    if args.bit < 0 or args.bit > 7:
        raise ValueError("bit must be in range 0..7")
    if args.value not in (0, 1):
        raise ValueError("value must be 0 or 1")

    base = list(read_6200_triplet(node))
    idx = args.sub - 1
    mask = 1 << args.bit

    tx = base.copy()
    tx[idx] = (tx[idx] & (~mask & 0xFF)) | (args.value << args.bit)

    print_sub_map(args.sub)
    print_control_target(args.sub, args.bit)
    print(f"[TX] sub={args.sub} bit={args.bit} write={args.value}")
    send_rpdo1(network, args.rpdo_cobid, tx[0], tx[1], tx[2])
    time.sleep(args.settle_delay)

    if args.readback:
        rb = list(read_6200_triplet(node))
        rb_bit = (rb[idx] >> args.bit) & 0x1
        ok = rb_bit == args.value
        tag = "PASS" if ok else "FAIL"
        print(f"[{tag}] sub={args.sub} bit={args.bit} read={rb_bit} expect={args.value}")
        if args.manual_confirm and not ask_continue():
            print("[INFO] Aborted by user")
            return 2
        return 0 if ok else 1

    if args.manual_confirm and not ask_continue():
        print("[INFO] Aborted by user")
        return 2

    return 0


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
    ap = argparse.ArgumentParser(description="RPDO1 DO control test for 0x6200.1/.2/.3")
    ap.add_argument("bustype", nargs="?", choices=("ixxat", "pcan"), default=None, help="CAN backend to use: ixxat or pcan")
    ap.add_argument("--bustype", dest="bustype_opt", choices=("ixxat", "pcan"), default=None, help="CAN backend to use: ixxat or pcan")
    ap.add_argument("--node-id", type=parse_int, default=0x34)
    ap.add_argument("--channel", default=None, help="CAN interface channel (int for IXXAT, string for PCAN, e.g. PCAN_USBBUS1)")
    ap.add_argument("--bitrate", type=int, default=500000)
    ap.add_argument("--rpdo-cobid", type=parse_int, default=None, help="Default: 0x200 + node-id")
    ap.add_argument("--settle-delay", type=float, default=0.05)
    ap.add_argument("--no-readback", dest="readback", action="store_false", help="Skip SDO readback")
    ap.add_argument("--no-manual-confirm", dest="manual_confirm", action="store_false", help="Do not prompt for level check")

    mode = ap.add_mutually_exclusive_group(required=False)
    mode.add_argument("--payload", help="Send RPDO1 payload once, e.g. 0x01,0x00,0x40")
    mode.add_argument("--pulse", action="store_true", help="Pulse one bit with 0->1->0 sequence")
    mode.add_argument("--all-bits", action="store_true", help="Pulse all mapped bits of 0x6200.1/.2/.3")

    ap.add_argument("sub", nargs="?", type=parse_int, default=3, help="Target sub-index (1..3)")
    ap.add_argument("bit", nargs="?", type=parse_int, default=0, help="Target bit (0..7)")
    ap.add_argument("value", nargs="?", type=parse_int, default=None, help="Single-write value (0 or 1). Example: pcan 3 0 1")
    ap.add_argument("--no-restore", dest="restore", action="store_false", help="Do not restore baseline after --pulse")

    ap.set_defaults(readback=True, restore=True, manual_confirm=True)
    return ap


def main() -> int:
    args = build_arg_parser().parse_args()
    args.bustype = args.bustype_opt if args.bustype_opt is not None else (args.bustype or "ixxat")
    if args.rpdo_cobid is None:
        args.rpdo_cobid = 0x200 + int(args.node_id)

    network = canopen.Network()
    node = canopen.RemoteNode(args.node_id, None)
    network.add_node(node)

    try:
        connect_network(network, args.bustype, args.channel, args.bitrate)
        node.nmt.send_command(1)
        time.sleep(0.1)

        if args.value is not None:
            return run_set_mode(network, node, args)

        if args.payload:
            return run_payload_mode(network, node, args)
        if args.all_bits:
            return run_all_bits_mode(network, node, args)
        return run_pulse_mode(network, node, args)
    finally:
        try:
            network.disconnect()
        except Exception as exc:
            print(f"[WARN] network.disconnect failed: {exc}")


if __name__ == "__main__":
    raise SystemExit(main())
