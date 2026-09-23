#!/usr/bin/env python3
import argparse
import time

import canopen


def parse_int(x) -> int:
    if isinstance(x, int):
        return x
    return int(str(x), 0)


def read_u32(node, index: int, sub: int) -> int:
    raw = node.sdo.upload(index, sub)
    b = raw if isinstance(raw, (bytes, bytearray)) else bytes(raw)
    return int.from_bytes(b[:4], byteorder="little", signed=False) if b else 0


def write_u32(node, index: int, sub: int, value: int) -> None:
    node.sdo.download(index, sub, int(value & 0xFFFFFFFF).to_bytes(4, "little", signed=False))


def build_arg_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(description="SDO helper for PWM carrier frequency (0x5402:0)")
    ap.add_argument("--node-id", type=parse_int, default=0x30)
    ap.add_argument("--bustype", default="ixxat")
    ap.add_argument("--channel", type=int, default=0)
    ap.add_argument("--bitrate", type=int, default=500000)

    ap.add_argument("--set", dest="set_freq", type=parse_int, default=None, help="Set 0x5402:0 (Hz)")
    ap.add_argument("--get", action="store_true", help="Read 0x5402:0")
    ap.add_argument("--verify", action="store_true", help="After --set, read back and compare")
    ap.add_argument("--settle-delay", type=float, default=0.05, help="Delay after write before readback")
    return ap


def main() -> int:
    args = build_arg_parser().parse_args()

    # If no explicit action is requested, do a read.
    if args.set_freq is None and not args.get:
        args.get = True

    network = canopen.Network()
    node = canopen.RemoteNode(args.node_id, None)
    network.add_node(node)

    try:
        network.connect(bustype=args.bustype, channel=args.channel, bitrate=args.bitrate)
        node.nmt.send_command(1)
        time.sleep(0.1)

        if args.set_freq is not None:
            freq = int(args.set_freq)
            if freq <= 0:
                print("[FAIL] Frequency must be > 0")
                return 1
            write_u32(node, 0x5402, 0, freq)
            print(f"[INFO] Wrote 0x5402:0 = {freq} Hz")

            if args.verify:
                time.sleep(args.settle_delay)
                rb = read_u32(node, 0x5402, 0)
                ok = rb == freq
                print(f"[{'PASS' if ok else 'FAIL'}] Readback 0x5402:0 = {rb} Hz")
                if not ok:
                    return 1

        if args.get:
            rb = read_u32(node, 0x5402, 0)
            print(f"[INFO] 0x5402:0 = {rb} Hz")

        return 0
    finally:
        try:
            network.disconnect()
        except Exception as exc:
            print(f"[WARN] network.disconnect failed: {exc}")


if __name__ == "__main__":
    raise SystemExit(main())
