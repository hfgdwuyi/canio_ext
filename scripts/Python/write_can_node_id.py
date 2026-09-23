import argparse
import time

import can
import canopen


SAVE_SIGNATURE = 0x65766173
NODE_TABLE_INDEX = 0x2101
BAUDRATE_TABLE_INDEX = 0x2102
SAVE_INDEX = 0x1010
SAVE_ALL_SUBINDEX = 0x01
NMT_RESET_NODE = 0x81
NMT_RESET_COMMUNICATION = 0x82


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


def read_u8(node, index: int, subindex: int) -> int:
    raw = node.sdo.upload(index, subindex)
    return raw[0] if isinstance(raw, (bytes, bytearray)) else int(raw) & 0xFF


def write_u8(node, index: int, subindex: int, value: int) -> None:
    node.sdo.download(index, subindex, bytes([value & 0xFF]))


def write_u32(node, index: int, subindex: int, value: int) -> None:
    node.sdo.download(index, subindex, int(value & 0xFFFFFFFF).to_bytes(4, byteorder="little", signed=False))


def read_u16(node, index: int, subindex: int) -> int:
    raw = node.sdo.upload(index, subindex)
    if isinstance(raw, int):
        return raw & 0xFFFF
    return int.from_bytes(bytes(raw)[:2], byteorder="little", signed=False)


def write_u16(node, index: int, subindex: int, value: int) -> None:
    node.sdo.download(index, subindex, int(value & 0xFFFF).to_bytes(2, byteorder="little", signed=False))


def send_nmt_reset_communication(network: canopen.Network, node_id: int) -> None:
    network.send_message(0x000, bytes([NMT_RESET_COMMUNICATION, node_id & 0x7F]))


def send_nmt_reset_node(network: canopen.Network, node_id: int) -> None:
    network.send_message(0x000, bytes([NMT_RESET_NODE, node_id & 0x7F]))


def read_sw_version(node) -> str:
    return node.sdo.upload(0x100A, 0x00).decode("ascii", errors="ignore").rstrip("\x00")


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Write CAN node-id defaults in 0x2101 and optionally persist them")
    parser.add_argument("bustype", nargs="?", choices=("ixxat", "pcan"), default=None, help="CAN backend to use: ixxat or pcan")
    parser.add_argument("--bustype", dest="bustype_opt", choices=("ixxat", "pcan"), default=None, help="CAN backend to use: ixxat or pcan")
    parser.add_argument("--node-id", type=parse_int, default=0x30, help="Current CANopen node id used to reach the device")
    parser.add_argument("--new-node-id", type=parse_int, default=0x34, help="Node id value to store into 0x2101:01..04")
    parser.add_argument("--new-baudrate", type=int, default=500, help="Baudrate value to store into 0x2102:01..04 (CANopen OD units, e.g. 500 for 500 kbit/s)")
    parser.add_argument("--channel", default=None, help="CAN interface channel (int for IXXAT, string for PCAN, e.g. PCAN_USBBUS1)")
    parser.add_argument("--bitrate", type=int, default=500000, help="CAN bitrate")
    parser.add_argument("--sdo-timeout", type=float, default=2.0, help="SDO response timeout in seconds")
    parser.add_argument("--sdo-retries", type=int, default=5, help="SDO retry count")
    parser.add_argument("--no-store", action="store_true", help="Write 0x2101 in RAM only and skip EEPROM persistence")
    parser.add_argument("--verify-only", action="store_true", help="Read and print 0x2101:01..04 without writing")
    parser.add_argument("--reset-comm", action="store_true", help="After saving, send NMT reset communication and verify the node still responds on the current node id")
    parser.add_argument("--reset-node", action="store_true", help="After saving, send NMT reset node and reconnect on the new node id")
    parser.add_argument("--reset-wait", type=float, default=2.0, help="Wait time after reset before reconnect")
    return parser


def main() -> None:
    args = build_arg_parser().parse_args()
    bustype = args.bustype_opt if args.bustype_opt is not None else (args.bustype or "ixxat")

    if args.new_node_id < 1 or args.new_node_id > 0x7F:
        raise ValueError("--new-node-id must be in range 1..0x7F")
    if args.new_baudrate < 1 or args.new_baudrate > 1000:
        raise ValueError("--new-baudrate must be in range 1..1000")
    if args.reset_comm and args.reset_node:
        raise ValueError("Use either --reset-comm or --reset-node, not both")

    network = canopen.Network()
    channel = connect_network(network, bustype, args.channel, args.bitrate)

    try:
        node = canopen.RemoteNode(args.node_id, None)
        network.add_node(node)
        node.sdo.RESPONSE_TIMEOUT = args.sdo_timeout
        node.sdo.MAX_RETRIES = args.sdo_retries

        node.nmt.send_command(128)
        time.sleep(0.1)

        print(
            f"=== CAN node-id config: current-node=0x{args.node_id:02X}, "
            f"target-default=0x{args.new_node_id:02X}, target-baudrate={args.new_baudrate}k, bustype={bustype}, channel={channel} ==="
        )

        before = [read_u8(node, NODE_TABLE_INDEX, subindex) for subindex in range(1, 5)]
        before_baud = [read_u16(node, BAUDRATE_TABLE_INDEX, subindex) for subindex in range(1, 5)]
        print("Current 0x2101 values:", ", ".join(f"sub{subindex}=0x{value:02X}" for subindex, value in enumerate(before, start=1)))
        print("Current 0x2102 values:", ", ".join(f"sub{subindex}={value}k" for subindex, value in enumerate(before_baud, start=1)))

        if not args.verify_only:
            for subindex in range(1, 5):
                write_u8(node, NODE_TABLE_INDEX, subindex, args.new_node_id)
                write_u16(node, BAUDRATE_TABLE_INDEX, subindex, args.new_baudrate)

            after = [read_u8(node, NODE_TABLE_INDEX, subindex) for subindex in range(1, 5)]
            after_baud = [read_u16(node, BAUDRATE_TABLE_INDEX, subindex) for subindex in range(1, 5)]
            print("Updated 0x2101 values:", ", ".join(f"sub{subindex}=0x{value:02X}" for subindex, value in enumerate(after, start=1)))
            print("Updated 0x2102 values:", ", ".join(f"sub{subindex}={value}k" for subindex, value in enumerate(after_baud, start=1)))

            if any(value != args.new_node_id for value in after):
                raise RuntimeError("0x2101 readback does not match requested node id")
            if any(value != args.new_baudrate for value in after_baud):
                raise RuntimeError("0x2102 readback does not match requested baudrate")

            if args.no_store:
                print("Skipped EEPROM save. New defaults are in RAM only until reboot/reset.")
            else:
                print("Persisting all parameters via 0x1010:01 = 'save' so 0x2101 is stored to EEPROM ...")
                write_u32(node, SAVE_INDEX, SAVE_ALL_SUBINDEX, SAVE_SIGNATURE)
                print("EEPROM save command sent.")

                if args.reset_comm:
                    print(f"Sending NMT reset communication to node 0x{args.node_id:02X} ...")
                    send_nmt_reset_communication(network, args.node_id)
                    time.sleep(args.reset_wait)

                    network.disconnect()
                    time.sleep(0.2)

                    verify_network = canopen.Network()
                    try:
                        verify_channel = connect_network(verify_network, bustype, args.channel, args.bitrate)
                        verify_node = canopen.RemoteNode(args.node_id, None)
                        verify_network.add_node(verify_node)
                        verify_node.sdo.RESPONSE_TIMEOUT = args.sdo_timeout
                        verify_node.sdo.MAX_RETRIES = args.sdo_retries
                        sw_version = read_sw_version(verify_node)
                        print(
                            f"Reset communication verified on current node 0x{args.node_id:02X} "
                            f"via {bustype}/{verify_channel}: 0x100A='{sw_version}'. "
                            f"Stored default node id remains 0x{args.new_node_id:02X} and takes effect after node reset/power-cycle."
                        )
                        return
                    finally:
                        verify_network.disconnect()
                elif args.reset_node:
                    print(f"Sending NMT reset node to node 0x{args.node_id:02X} ...")
                    send_nmt_reset_node(network, args.node_id)
                    time.sleep(args.reset_wait)

                    network.disconnect()
                    time.sleep(0.2)

                    verify_network = canopen.Network()
                    try:
                        verify_channel = connect_network(verify_network, bustype, args.channel, args.bitrate)
                        verify_node = canopen.RemoteNode(args.new_node_id, None)
                        verify_network.add_node(verify_node)
                        verify_node.sdo.RESPONSE_TIMEOUT = args.sdo_timeout
                        verify_node.sdo.MAX_RETRIES = args.sdo_retries
                        sw_version = read_sw_version(verify_node)
                        print(
                            f"Reset node verified on new node 0x{args.new_node_id:02X} "
                            f"via {bustype}/{verify_channel}: 0x100A='{sw_version}'"
                        )
                        return
                    finally:
                        verify_network.disconnect()
                else:
                    print("Power-cycle or use --reset-node to apply stored defaults to the active CAN node id.")
    finally:
        network.disconnect()


if __name__ == "__main__":
    main()