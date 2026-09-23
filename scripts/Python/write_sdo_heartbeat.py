
import argparse
import time

import canopen
import can


def parse_int(x) -> int:
    if isinstance(x, int):
        return x
    return int(str(x), 0)


def resolve_channel(bustype: str, channel_value) -> object:
    if channel_value is not None:
        return channel_value
    return "PCAN_USBBUS1" if bustype == "pcan" else 0


def build_connect_kwargs(bustype: str, channel_value, bitrate: int) -> dict:
    channel = resolve_channel(bustype, channel_value)
    if bustype == "pcan":
        return {"interface": "pcan", "channel": channel, "bitrate": bitrate}
    return {"bustype": "ixxat", "channel": channel, "bitrate": bitrate}


def _connect_network(network: canopen.Network, bustype: str, channel_value, bitrate: int) -> None:
    if bustype != "pcan":
        connect_kwargs = build_connect_kwargs(bustype, channel_value, bitrate)
        try:
            network.connect(**connect_kwargs)
        except TypeError:
            network.connect(bustype="ixxat", channel=connect_kwargs["channel"], bitrate=bitrate)
        return

    # If user provides --channel, honor it directly.
    if channel_value is not None:
        connect_kwargs = build_connect_kwargs("pcan", channel_value, bitrate)
        try:
            network.connect(**connect_kwargs)
            return
        except TypeError:
            network.connect(bustype="pcan", channel=connect_kwargs["channel"], bitrate=bitrate)
            return

    # Auto-try detected PCAN channels first, then common defaults.
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
    ap = argparse.ArgumentParser(description="Write 0x1017 producer heartbeat time over SDO")
    ap.add_argument("bustype", nargs="?", choices=("ixxat", "pcan"), default=None, help="CAN backend to use: ixxat or pcan")
    ap.add_argument("--bustype", dest="bustype_opt", choices=("ixxat", "pcan"), default=None, help="CAN backend to use: ixxat or pcan")
    ap.add_argument("--node-id", type=parse_int, default=0x30, help="CANopen node id")
    ap.add_argument("--channel", default=None, help="CAN interface channel (int for IXXAT, string for PCAN, e.g. PCAN_USBBUS1)")
    ap.add_argument("--bitrate", type=int, default=500000, help="CAN bitrate")
    ap.add_argument("heartbeat_ms", nargs="?", type=int, default=2000, help="Producer heartbeat time in milliseconds")
    return ap


def main():
    args = build_arg_parser().parse_args()
    bustype = args.bustype_opt if args.bustype_opt is not None else (args.bustype or "ixxat")
    node_id = args.node_id
    heartbeat_time_ms = args.heartbeat_ms

    network = canopen.Network()
    _connect_network(network, bustype, args.channel, args.bitrate)

    try:
        node = canopen.RemoteNode(node_id, None)
        network.add_node(node)
        node.sdo.RESPONSE_TIMEOUT = 2.0
        node.sdo.MAX_RETRIES = 5

        node.nmt.send_command(1)
        time.sleep(0.1)

        node.sdo.download(0x1017, 0x00, heartbeat_time_ms.to_bytes(2, byteorder="little", signed=False))
        print(f"Set node 0x{node_id:02X} heartbeat period to {heartbeat_time_ms} ms by SDO")

        raw = node.sdo.upload(0x1017, 0x00)
        value = int.from_bytes(raw, byteorder="little", signed=False)
        print(f"Read back heartbeat period: {value} ms")
    finally:
        network.disconnect()

if __name__ == "__main__":
    main()