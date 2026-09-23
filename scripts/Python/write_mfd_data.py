import argparse
import time

import can
import canopen


SAVE_SIGNATURE = 0x65766173
WRITE_DEVICE_NAME_INDEX = 0x2008
WRITE_HW_VERSION_INDEX = 0x2009
WRITE_SW_VERSION_INDEX = 0x200A
WRITE_IDENTITY_INDEX = 0x2018
COMPONENT_SERIAL_INDEX = 0x2019
PCBA_PART_NUMBER_INDEX = 0x201A
ACTIVE_DEVICE_NAME_INDEX = 0x1008
ACTIVE_HW_VERSION_INDEX = 0x1009
ACTIVE_SW_VERSION_INDEX = 0x100A
IDENTITY_INDEX = 0x1018
SAVE_INDEX = 0x1010
SAVE_ASSET_SUBINDEX = 0x04
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


def safe_disconnect(network: canopen.Network) -> None:
    try:
        network.disconnect()
    except Exception as exc:
        print(f"Ignoring disconnect error after CAN activity: {exc}")


def read_u32(node, index: int, subindex: int) -> int:
    raw = node.sdo.upload(index, subindex)
    if isinstance(raw, int):
        return raw & 0xFFFFFFFF
    return int.from_bytes(bytes(raw)[:4], byteorder="little", signed=False)


def write_u32(node, index: int, subindex: int, value: int) -> None:
    node.sdo.download(index, subindex, int(value & 0xFFFFFFFF).to_bytes(4, byteorder="little", signed=False))


def read_vis_string(node, index: int) -> str:
    raw = node.sdo.upload(index, 0x00)
    if isinstance(raw, str):
        return raw.rstrip("\x00")
    return bytes(raw).decode("ascii", errors="ignore").rstrip("\x00")


def write_vis_string(node, index: int, value: str) -> None:
    encoded = value.encode("ascii")
    if len(encoded) > 31:
        raise ValueError(f"String for 0x{index:04X} is too long. Max 31 ASCII characters.")
    node.sdo.download(index, 0x00, encoded + b"\x00")


def encode_revision(revision_pcba: int, revision_component: int) -> int:
    if revision_pcba < 0 or revision_pcba > 0xFFFF:
        raise ValueError("--revision-pcba must be in range 0..65535")
    if revision_component < 0 or revision_component > 0xFFFF:
        raise ValueError("--revision-component must be in range 0..65535")
    return ((revision_pcba & 0xFFFF) << 16) | (revision_component & 0xFFFF)


def decode_revision(revision_value: int) -> tuple[int, int]:
    return (revision_value >> 16) & 0xFFFF, revision_value & 0xFFFF


def send_nmt_reset_node(network: canopen.Network, node_id: int) -> None:
    network.send_message(0x000, bytes([NMT_RESET_NODE, node_id & 0x7F]))


def print_identity(prefix: str, node, index: int) -> None:
    product_code = read_u32(node, index, 0x02)
    revision_value = read_u32(node, index, 0x03)
    serial_number = read_u32(node, index, 0x04)
    revision_pcba, revision_component = decode_revision(revision_value)
    print(f"{prefix} 0x{index:04X}:02 Part Number Component = 0x{product_code:08X} ({product_code})")
    print(f"{prefix} 0x{index:04X}:03 Revision PCBA         = 0x{revision_pcba:04X} ({revision_pcba})")
    print(f"{prefix} 0x{index:04X}:03 Revision Component    = 0x{revision_component:04X} ({revision_component})")
    print(f"{prefix} 0x{index:04X}:04 Serial Number PCBA    = 0x{serial_number:08X} ({serial_number})")


def print_component_serial(prefix: str, node) -> None:
    serial_number = read_u32(node, COMPONENT_SERIAL_INDEX, 0x01)
    print(
        f"{prefix} 0x{COMPONENT_SERIAL_INDEX:04X}:01 Serial Number Component = "
        f"0x{serial_number:08X} ({serial_number})"
    )


def print_pcba_part_number(prefix: str, node) -> None:
    part_number = read_u32(node, PCBA_PART_NUMBER_INDEX, 0x00)
    print(f"{prefix} 0x{PCBA_PART_NUMBER_INDEX:04X} Part Number PCBA       = 0x{part_number:08X} ({part_number})")


def print_stored_strings(prefix: str, node) -> None:
    print(f"{prefix} 0x{WRITE_DEVICE_NAME_INDEX:04X} Device Name            = '{read_vis_string(node, WRITE_DEVICE_NAME_INDEX)}'")
    print(f"{prefix} 0x{WRITE_HW_VERSION_INDEX:04X} Layout E-state PCB     = '{read_vis_string(node, WRITE_HW_VERSION_INDEX)}'")
    print(f"{prefix} 0x{WRITE_SW_VERSION_INDEX:04X} SW Version             = '{read_vis_string(node, WRITE_SW_VERSION_INDEX)}'")


def print_active_strings(prefix: str, node) -> None:
    print(f"{prefix} 0x{ACTIVE_DEVICE_NAME_INDEX:04X} Device Name            = '{read_vis_string(node, ACTIVE_DEVICE_NAME_INDEX)}'")
    print(f"{prefix} 0x{ACTIVE_HW_VERSION_INDEX:04X} Layout E-state PCB     = '{read_vis_string(node, ACTIVE_HW_VERSION_INDEX)}'")
    print(f"{prefix} 0x{ACTIVE_SW_VERSION_INDEX:04X} SW Version             = '{read_vis_string(node, ACTIVE_SW_VERSION_INDEX)}'")


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Write MFD asset data and read it back")
    parser.add_argument("bustype", nargs="?", choices=("ixxat", "pcan"), default=None, help="CAN backend to use: ixxat or pcan")
    parser.add_argument("--bustype", dest="bustype_opt", choices=("ixxat", "pcan"), default=None, help="CAN backend to use: ixxat or pcan")
    parser.add_argument("--node-id", type=parse_int, default=0x34, help="Current CANopen node id used to reach the device")
    parser.add_argument("--channel", default=None, help="CAN interface channel (int for IXXAT, string for PCAN, e.g. PCAN_USBBUS1)")
    parser.add_argument("--bitrate", type=int, default=500000, help="CAN bitrate")
    parser.add_argument("--sdo-timeout", type=float, default=2.0, help="SDO response timeout in seconds")
    parser.add_argument("--sdo-retries", type=int, default=5, help="SDO retry count")
    parser.add_argument("--device-name", default="CANIO_EXT", help="Value written to 0x2008 device name")
    parser.add_argument("--layout-e-state", default="E1", help="Value written to 0x2009 hardware version / PCB E-state")
    parser.add_argument("--sw-version", default="1.0.0 Application", help="Value written to 0x200A software version")
    parser.add_argument("--part-number-pcba", type=parse_int, default=11925826, help="Value written to 0x201A PCBA part number")
    parser.add_argument("--part-number-component", type=parse_int, default=11925830, help="Value written to 0x2018:02 productCode")
    parser.add_argument("--revision-pcba", type=parse_int, default=0, help="Upper 16 bits of 0x2018:03 revisionNumber")
    parser.add_argument("--revision-component", type=parse_int, default=0, help="Lower 16 bits of 0x2018:03 revisionNumber")
    parser.add_argument("--serial-number-pcba", type=parse_int, default=500014, help="Value written to 0x2018:04 serialNumber")
    parser.add_argument("--serial-number-component", type=parse_int, default=50014, help="Value written to 0x2019:01 component serial number")
    parser.add_argument("--no-store", action="store_true", help="Write 0x2018 in RAM only and skip EEPROM persistence")
    parser.add_argument("--reset-node", action="store_true", help="After saving, send NMT reset node and read back 0x1018")
    parser.add_argument("--reset-wait", type=float, default=2.0, help="Wait time after reset before reconnect")
    parser.add_argument("--verify-only", action="store_true", help="Only read and print 0x2018/0x1018 without writing")
    return parser


def main() -> None:
    args = build_arg_parser().parse_args()
    bustype = args.bustype_opt if args.bustype_opt is not None else (args.bustype or "ixxat")
    revision_value = encode_revision(args.revision_pcba, args.revision_component)

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
            f"=== MFD write/read: node=0x{args.node_id:02X}, bustype={bustype}, channel={channel}, bitrate={args.bitrate} ==="
        )
        print_stored_strings("Current stored", node)
        print_identity("Current stored", node, WRITE_IDENTITY_INDEX)
        print_pcba_part_number("Current stored", node)
        print_active_strings("Current active", node)
        print_identity("Current active", node, IDENTITY_INDEX)
        print_pcba_part_number("Current direct", node)
        print_component_serial("Current direct", node)

        if args.verify_only:
            return

        write_vis_string(node, WRITE_DEVICE_NAME_INDEX, args.device_name)
        write_vis_string(node, WRITE_HW_VERSION_INDEX, args.layout_e_state)
        write_vis_string(node, WRITE_SW_VERSION_INDEX, args.sw_version)
        write_u32(node, PCBA_PART_NUMBER_INDEX, 0x00, args.part_number_pcba)
        write_u32(node, WRITE_IDENTITY_INDEX, 0x02, args.part_number_component)
        write_u32(node, WRITE_IDENTITY_INDEX, 0x03, revision_value)
        write_u32(node, WRITE_IDENTITY_INDEX, 0x04, args.serial_number_pcba)
        write_u32(node, COMPONENT_SERIAL_INDEX, 0x01, args.serial_number_component)

        print_stored_strings("Updated stored", node)
        print_identity("Updated stored", node, WRITE_IDENTITY_INDEX)
        print_pcba_part_number("Updated stored", node)
        print_component_serial("Updated direct", node)

        if args.no_store:
            print("Skipped EEPROM save. Updated MFD data is in RAM only until reboot/reset.")
            return

        print("Persisting asset data via 0x1010:04 = 'save' ...")
        write_u32(node, SAVE_INDEX, SAVE_ASSET_SUBINDEX, SAVE_SIGNATURE)
        print("Asset-data save command sent.")

        if not args.reset_node:
            print("Use --reset-node or power-cycle the device so 0x1018 is refreshed from 0x2018.")
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
            print(f"Reconnected for verification via {bustype}/{verify_channel}")
            print_stored_strings("Verified stored", verify_node)
            print_identity("Verified stored", verify_node, WRITE_IDENTITY_INDEX)
            print_pcba_part_number("Verified stored", verify_node)
            print_active_strings("Verified active", verify_node)
            print_identity("Verified active", verify_node, IDENTITY_INDEX)
            print_pcba_part_number("Verified direct", verify_node)
            print_component_serial("Verified direct", verify_node)
        except Exception as exc:
            print(
                "MFD write/save succeeded, but post-reset verification failed. "
                f"This can happen during CAN interface recovery after node reset: {exc}"
            )
            print("Re-run this script with --verify-only or use read_asset_data.py after the node settles.")
        finally:
            safe_disconnect(verify_network)
    finally:
        safe_disconnect(network)


if __name__ == "__main__":
    main()