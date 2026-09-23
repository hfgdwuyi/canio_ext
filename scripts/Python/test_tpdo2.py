#!/usr/bin/env python3
"""Listen and print TPDO2 mapped values.

By default the script only listens for TPDO2 traffic.
Use --set-event-timer-ms to force periodic TPDO2 transmission through 0x1801:05.
Use --store-comm to save the written communication parameter via 0x1010:02.
"""

from __future__ import annotations

import argparse
import sys
import time
from threading import Lock

import can
import canopen


class CobCounter:
    def __init__(self, mapping: list[tuple[int, int, int]], print_frames: bool = False) -> None:
        self.count = 0
        self.last_payload = b""
        self.last_ts = 0.0
        self.frames: list[tuple[float, bytes]] = []
        self.mapping = mapping
        self.print_frames = print_frames
        self._lock = Lock()

    def callback_from_can(self, can_id: int, data: bytes, timestamp: float) -> None:
        payload = bytes(data)
        ts = float(timestamp)
        with self._lock:
            self.count += 1
            self.last_payload = payload
            self.last_ts = ts
            self.frames.append((ts, payload))

        if self.print_frames:
            render_tpdo2_frame(can_id, ts, payload, self.mapping)
        else:
            render_tpdo2_values_only(ts, payload, self.mapping)


def parse_int(value: str) -> int:
    return int(value, 0)


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
            channel = cfg.get("channel")
            if channel and channel not in candidates:
                candidates.append(channel)
    except Exception:
        pass

    for channel in ("PCAN_USBBUS1", "PCAN_USBBUS2", "PCAN_USBBUS3", "PCAN_USBBUS4"):
        if channel not in candidates:
            candidates.append(channel)

    last_exc = None
    for channel in candidates:
        try:
            network.connect(interface="pcan", channel=channel, bitrate=bitrate)
            print(f"PCAN connected on channel: {channel}")
            return
        except TypeError:
            try:
                network.connect(bustype="pcan", channel=channel, bitrate=bitrate)
                print(f"PCAN connected on channel: {channel}")
                return
            except Exception as exc:
                last_exc = exc
        except Exception as exc:
            last_exc = exc

    tried = ", ".join(str(channel) for channel in candidates) if candidates else "(none)"
    raise RuntimeError(
        "Failed to connect PCAN with auto channel detection. "
        f"Tried channels: {tried}. "
        "Please pass an explicit channel, e.g. --channel PCAN_USBBUS1"
    ) from last_exc


def decode_map_entry(entry: int) -> tuple[int, int, int]:
    index = (entry >> 16) & 0xFFFF
    sub = (entry >> 8) & 0xFF
    bits = entry & 0xFF
    return index, sub, bits


def read_u16(node: canopen.RemoteNode, index: int, sub: int) -> int:
    raw = node.sdo.upload(index, sub)
    if isinstance(raw, int):
        return raw & 0xFFFF
    return int.from_bytes(bytes(raw)[:2], "little", signed=False)


def read_u32(node: canopen.RemoteNode, index: int, sub: int) -> int:
    raw = node.sdo.upload(index, sub)
    if isinstance(raw, int):
        return raw & 0xFFFFFFFF
    return int.from_bytes(bytes(raw)[:4], "little", signed=False)


def write_u16(node: canopen.RemoteNode, index: int, sub: int, value: int) -> None:
    node.sdo.download(index, sub, int(value & 0xFFFF).to_bytes(2, "little", signed=False))


def write_u32(node: canopen.RemoteNode, index: int, sub: int, value: int) -> None:
    node.sdo.download(index, sub, int(value & 0xFFFFFFFF).to_bytes(4, "little", signed=False))


def store_comm_parameters(node: canopen.RemoteNode) -> None:
    write_u32(node, 0x1010, 0x02, 0x65766173)


def read_mapping(node: canopen.RemoteNode, map_index: int) -> list[tuple[int, int, int]]:
    count = node.sdo.upload(map_index, 0x00)
    if isinstance(count, int):
        count_value = count & 0xFF
    else:
        count_value = int.from_bytes(bytes(count)[:1], "little", signed=False)

    items: list[tuple[int, int, int]] = []
    for sub in range(1, count_value + 1):
        raw = node.sdo.upload(map_index, sub)
        if isinstance(raw, int):
            value = raw & 0xFFFFFFFF
        else:
            value = int.from_bytes(bytes(raw)[:4], "little", signed=False)
        items.append(decode_map_entry(value))
    return items


def decode_signed_le(raw: bytes, bits: int) -> int:
    if bits == 8:
        return int.from_bytes(raw[:1], "little", signed=True)
    if bits == 16:
        return int.from_bytes(raw[:2], "little", signed=True)
    if bits == 32:
        return int.from_bytes(raw[:4], "little", signed=True)
    raise ValueError(f"Unsupported mapping width: {bits}")


def decode_tpdo_values(payload: bytes, mapping: list[tuple[int, int, int]]) -> list[tuple[int, int, int, int]]:
    values: list[tuple[int, int, int, int]] = []
    bit_offset = 0
    for index, sub, bits in mapping:
        if bits not in (8, 16, 32):
            bit_offset += bits
            continue
        if bit_offset % 8 != 0:
            bit_offset += bits
            continue
        byte_offset = bit_offset // 8
        byte_len = bits // 8
        if byte_offset + byte_len > len(payload):
            break
        part = payload[byte_offset : byte_offset + byte_len]
        value = decode_signed_le(part, bits)
        values.append((index, sub, bits, value))
        bit_offset += bits
    return values


def render_tpdo2_frame(cobid: int, timestamp: float, payload: bytes, mapping: list[tuple[int, int, int]]) -> None:
    ts_sec = int(timestamp)
    ts_millis = int((timestamp - ts_sec) * 1000)
    ts_text = time.strftime("%H:%M:%S", time.localtime(ts_sec)) + f".{ts_millis:03d}"
    print(f"[TPDO2] cobid=0x{cobid:03X} ts={ts_text} data=[{' '.join(f'{byte:02X}' for byte in payload)}]")
    for index, sub, bits, value in decode_tpdo_values(payload, mapping):
        print(f"  0x{index:04X}:{sub:02X} ({bits}b) = {value}")


def render_tpdo2_values_only(timestamp: float, payload: bytes, mapping: list[tuple[int, int, int]]) -> None:
    ts_sec = int(timestamp)
    ts_millis = int((timestamp - ts_sec) * 1000)
    ts_text = time.strftime("%H:%M:%S", time.localtime(ts_sec)) + f".{ts_millis:03d}"
    for index, sub, bits, value in decode_tpdo_values(payload, mapping):
        print(f"{ts_text} 0x{index:04X}:{sub:02X} ({bits}b) = {value}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Listen and print TPDO2 mapped values")
    parser.add_argument("bustype", nargs="?", choices=("ixxat", "pcan"), default=None, help="CAN backend to use: ixxat or pcan")
    parser.add_argument("--bustype", dest="bustype_opt", choices=("ixxat", "pcan"), default=None, help="CAN backend to use: ixxat or pcan")
    parser.add_argument("--node-id", type=parse_int, default=0x30, help="CANopen node ID (default: 0x30)")
    parser.add_argument("--channel", default=None, help="CAN interface channel (int for IXXAT, string for PCAN, e.g. PCAN_USBBUS1)")
    parser.add_argument("--bitrate", type=int, default=500000, help="CAN bitrate")
    parser.add_argument("--listen-sec", type=float, default=8.0, help="Seconds to listen for TPDO2 frames")
    parser.add_argument("--print-frames", action="store_true", help="Verbose TPDO2 frame output")
    parser.add_argument("--set-event-timer-ms", type=int, default=None, help="Write TPDO2 event timer at 0x1801:05 before listening")
    parser.add_argument("--restore-event-timer", action="store_true", help="Restore original 0x1801:05 on exit when --store-comm is not used")
    parser.add_argument("--store-comm", action="store_true", help="Store communication params by writing 'save' to 0x1010:02")
    parser.add_argument("--no-start-node", action="store_true", help="Skip sending NMT Operational before listening")
    parser.add_argument("--verbose", action="store_true", help="Print diagnostics")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    args.bustype = args.bustype_opt if args.bustype_opt is not None else (args.bustype or "ixxat")

    network = canopen.Network()
    node = canopen.RemoteNode(args.node_id, None)
    network.add_node(node)

    original_event_timer = None
    try:
        connect_network(network, args.bustype, args.channel, args.bitrate)

        if not args.no_start_node:
            node.nmt.send_command(1)
            time.sleep(0.1)

        cobid = read_u32(node, 0x1801, 0x01)
        mapping = read_mapping(node, 0x1A01)
        original_event_timer = read_u16(node, 0x1801, 0x05)

        if args.verbose:
            transmission_type = node.sdo.upload(0x1801, 0x02)
            if isinstance(transmission_type, int):
                transmission_type &= 0xFF
            else:
                transmission_type = int.from_bytes(bytes(transmission_type)[:1], "little", signed=False)
            print(f"INFO: TPDO2 COB-ID=0x{(cobid & 0x7FF):03X}, transmission type=0x{transmission_type:02X}, event timer={original_event_timer} ms")

        if args.set_event_timer_ms is not None:
            if args.set_event_timer_ms < 0 or args.set_event_timer_ms > 0xFFFF:
                print("ERROR: --set-event-timer-ms out of range (0..65535)", file=sys.stderr)
                return 2
            write_u16(node, 0x1801, 0x05, args.set_event_timer_ms)
            time.sleep(0.05)
            if args.verbose:
                readback = read_u16(node, 0x1801, 0x05)
                print(f"INFO: wrote TPDO2 event timer to {readback} ms")

        if args.store_comm:
            store_comm_parameters(node)
            if args.verbose:
                print("INFO: stored communication parameters via 0x1010:02='save'")

        pdo_id = cobid & 0x7FF
        tpdo_counter = CobCounter(mapping, print_frames=args.print_frames)
        network.subscribe(pdo_id, tpdo_counter.callback_from_can)

        end_time = time.time() + args.listen_sec
        while time.time() < end_time:
            time.sleep(0.05)

        if args.verbose:
            print(f"INFO: received {tpdo_counter.count} TPDO2 frame(s)")
        return 0 if tpdo_counter.count > 0 else 1
    except KeyboardInterrupt:
        return 130
    except Exception as exc:
        print(f"ERROR: TPDO2 runtime error: {exc}", file=sys.stderr)
        return 2
    finally:
        if (
            args.restore_event_timer
            and (not args.store_comm)
            and original_event_timer is not None
            and args.set_event_timer_ms is not None
        ):
            try:
                write_u16(node, 0x1801, 0x05, original_event_timer)
                time.sleep(0.05)
                if args.verbose:
                    restored = read_u16(node, 0x1801, 0x05)
                    print(f"INFO: restored TPDO2 event timer to {restored} ms")
            except Exception:
                pass
        try:
            network.disconnect()
        except Exception:
            pass


if __name__ == "__main__":
    raise SystemExit(main())