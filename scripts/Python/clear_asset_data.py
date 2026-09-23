import argparse
import time

import can
import canopen


LOAD_SIGNATURE = 0x64616F6C
RESTORE_INDEX = 0x1011
RESTORE_ASSET_SUBINDEX = 0x04
NMT_RESET_NODE = 0x81


def parse_int(value) -> int:
    if isinstance(value, int):
        return value
    return int(str(value), 0)


def resolve_channel(bustype: str, channel_value) -> object:
    if channel_value is not None:
        return channel_value
    return "PCAN_USBBUS1" if bustype == "pcan" else 0


def connect_network(network: canopen.Network, bustype: str, channel_value, bitrate: int) -> object:
    if bustype != "pcan":
        channel = resolve_channel(bustype, channel_value)
        try:
            network.connect(bustype=bustype, channel=channel, bitrate=bitrate)
        except TypeError:
            network.connect(bustype=bustype, channel=channel, bitrate=bitrate)
        return channel

    if channel_value is not None:
        try:
            network.connect(interface="pcan", channel=channel_value, bitrate=bitrate)
        except TypeError:
            network.connect(bustype="pcan", channel=channel_value, bitrate=bitrate)
        return channel_value

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
            return channel
        except TypeError:
            try:
                network.connect(bustype="pcan", channel=channel, bitrate=bitrate)
                print(f"PCAN connected on channel: {channel}")
                return channel
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


def write_u32(node, index: int, subindex: int, value: int) -> None:
    node.sdo.download(index, subindex, int(value & 0xFFFFFFFF).to_bytes(4, byteorder="little", signed=False))


def read_identity_u32(node, subindex: int) -> int:
    raw = node.sdo.upload(0x1018, subindex)
    if isinstance(raw, int):
        return raw & 0xFFFFFFFF
    return int.from_bytes(bytes(raw)[:4], byteorder="little", signed=False)


def send_nmt_reset_node(network: canopen.Network, node_id: int) -> None:
    network.send_message(0x000, bytes([NMT_RESET_NODE, node_id & 0x7F]))


def safe_disconnect(network: canopen.Network) -> None:
    try:
        network.disconnect()
    except Exception as exc:
        print(f"Ignoring disconnect error after CAN reset: {exc}")


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Clear CANopen asset-data defaults in EEPROM via 0x1011:04")
    parser.add_argument("bustype", nargs="?", choices=("ixxat", "pcan"), default=None, help="CAN backend to use: ixxat or pcan")
    parser.add_argument("--bustype", dest="bustype_opt", choices=("ixxat", "pcan"), default=None, help="CAN backend to use: ixxat or pcan")
    parser.add_argument("--node-id", type=parse_int, default=0x34, help="Current CANopen node id used to reach the device")
    parser.add_argument("--channel", default=None, help="CAN interface channel (int for IXXAT, string for PCAN, e.g. PCAN_USBBUS1)")
    parser.add_argument("--bitrate", type=int, default=500000, help="CAN bitrate")
    parser.add_argument("--sdo-timeout", type=float, default=2.0, help="SDO response timeout in seconds")
    parser.add_argument("--sdo-retries", type=int, default=5, help="SDO retry count")
    parser.add_argument("--reset-node", action="store_true", help="Send NMT reset node after clearing asset EEPROM")
    parser.add_argument("--reset-wait", type=float, default=2.0, help="Wait time after reset before reconnect")
    parser.add_argument("--verify-only", action="store_true", help="Only read and print 0x1018:02..04 without writing 0x1011:04")
    return parser


def main() -> None:
    args = build_arg_parser().parse_args()
    bustype = args.bustype_opt if args.bustype_opt is not None else (args.bustype or "ixxat")

    network = canopen.Network()
    channel = connect_network(network, bustype, args.channel, args.bitrate)

    try:
        node = canopen.RemoteNode(args.node_id, None)
        network.add_node(node)
        node.sdo.RESPONSE_TIMEOUT = args.sdo_timeout
        node.sdo.MAX_RETRIES = args.sdo_retries

        node.nmt.send_command(128)
        time.sleep(0.1)

        product_code = read_identity_u32(node, 0x02)
        revision = read_identity_u32(node, 0x03)
        serial_number = read_identity_u32(node, 0x04)
        print(
            f"=== Asset data: node=0x{args.node_id:02X}, bustype={bustype}, channel={channel}, bitrate={args.bitrate} ==="
        )
        print(f"Current 0x1018:02 Product Code = 0x{product_code:08X} ({product_code})")
        print(f"Current 0x1018:03 Revision    = 0x{revision:08X} ({revision})")
        print(f"Current 0x1018:04 Serial No.  = 0x{serial_number:08X} ({serial_number})")

        if args.verify_only:
            return

        print("Clearing asset-data EEPROM via 0x1011:04 = 'load' ...")
        write_u32(node, RESTORE_INDEX, RESTORE_ASSET_SUBINDEX, LOAD_SIGNATURE)
        print("Asset-data clear command sent.")

        if not args.reset_node:
            print("Use --reset-node or power-cycle the device so defaults are reloaded from firmware.")
            return

        print(f"Sending NMT reset node to node 0x{args.node_id:02X} ...")
        send_nmt_reset_node(network, args.node_id)
        time.sleep(args.reset_wait)

        safe_disconnect(network)
        time.sleep(0.2)

        verify_network = canopen.Network()
        try:
            verify_channel = connect_network(verify_network, bustype, args.channel, args.bitrate)
            verify_node = canopen.RemoteNode(args.node_id, None)
            verify_network.add_node(verify_node)
            verify_node.sdo.RESPONSE_TIMEOUT = args.sdo_timeout
            verify_node.sdo.MAX_RETRIES = args.sdo_retries

            try:
                product_code = read_identity_u32(verify_node, 0x02)
                revision = read_identity_u32(verify_node, 0x03)
                serial_number = read_identity_u32(verify_node, 0x04)
                print(
                    f"After reset via {bustype}/{verify_channel}: 0x1018:02=0x{product_code:08X}, "
                    f"0x1018:03=0x{revision:08X}, 0x1018:04=0x{serial_number:08X}"
                )
            except Exception as exc:
                print(
                    "Asset-data clear was accepted, but post-reset verification failed. "
                    f"This can happen during IXXAT bus recovery after node reset: {exc}"
                )
                print("Re-run read_asset_data.py after the node settles to confirm the new values.")
        finally:
            safe_disconnect(verify_network)
    finally:
        safe_disconnect(network)


if __name__ == "__main__":
    main()