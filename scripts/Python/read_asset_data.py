
import argparse
import time

import can
import canopen


COMPONENT_SERIAL_INDEX = 0x2019
PCBA_PART_NUMBER_INDEX = 0x201A


def parse_int(x) -> int:
    if isinstance(x, int):
        return x
    return int(str(x), 0)


def resolve_channel(bustype: str, channel_value) -> object:
    if channel_value is not None:
        return channel_value
    return "PCAN_USBBUS1" if bustype == "pcan" else 0


def _connect_network(network: canopen.Network, bustype: str, channel_value, bitrate: int) -> None:
    if bustype != "pcan":
        try:
            network.connect(bustype=bustype, channel=resolve_channel(bustype, channel_value), bitrate=bitrate)
            return
        except TypeError:
            network.connect(bustype=bustype, channel=resolve_channel(bustype, channel_value), bitrate=bitrate)
            return

    if channel_value is not None:
        try:
            network.connect(interface="pcan", channel=channel_value, bitrate=bitrate)
            return
        except TypeError:
            network.connect(bustype="pcan", channel=channel_value, bitrate=bitrate)
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


def is_sdo_no_object_error(exc: Exception) -> bool:
    code = getattr(exc, "code", None)
    if code == 0x06020000:
        return True
    text = str(exc).lower()
    return ("0x06020000" in text) or ("object does not exist" in text)


class ObjReader:
    def __init__(self, node_id, bustype, channel, bitrate):
        self.network = canopen.Network()
        self.node = canopen.RemoteNode(node_id, None)  # No EDS required
        self.network.add_node(self.node)
        _connect_network(self.network, bustype, channel, bitrate)
        # Switch to OPERATIONAL once before reading objects.
        self.node.nmt.send_command(1)
        time.sleep(0.1)

    def close(self):
        try:
            self.network.disconnect()
        except Exception as e:
            print(f"network.disconnect error (can be ignored): {e}")

    def upload_raw(self, index, subindex):
        return self.node.sdo.upload(index, subindex)

    @staticmethod
    def as_u32(raw) -> int:
        if isinstance(raw, int):
            return raw & 0xFFFFFFFF
        if isinstance(raw, (bytes, bytearray)):
            return int.from_bytes(raw, byteorder="little", signed=False) & 0xFFFFFFFF
        b = bytes(raw)
        return int.from_bytes(b, byteorder="little", signed=False) & 0xFFFFFFFF

    @staticmethod
    def as_str(raw) -> str:
        if isinstance(raw, str):
            return raw.rstrip("\x00")
        if isinstance(raw, (bytes, bytearray)):
            return raw.decode("ascii", errors="ignore").rstrip("\x00")
        return bytes(raw).decode("ascii", errors="ignore").rstrip("\x00")

    def read_str(self, index: int, max_len: int | None = None) -> str:
        raw = self.upload_raw(index, 0)
        s = self.as_str(raw)
        if max_len is not None:
            s = s[:max_len]
        return s

    def read_u32(self, index: int, subindex: int) -> int:
        raw = self.upload_raw(index, subindex)
        return self.as_u32(raw)


def decode_revision(revision_value: int) -> tuple[int, int]:
    return (revision_value >> 16) & 0xFFFF, revision_value & 0xFFFF


def build_arg_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(description="Read selected CANopen asset data objects over SDO")
    ap.add_argument("bustype", nargs="?", choices=("ixxat", "pcan"), default=None, help="CAN backend to use: ixxat or pcan")
    ap.add_argument("--bustype", dest="bustype_opt", choices=("ixxat", "pcan"), default=None, help="CAN backend to use: ixxat or pcan")
    ap.add_argument("--node-id", type=parse_int, default=0x34, help="CANopen node id")
    ap.add_argument("--channel", default=None, help="CAN interface channel (int for IXXAT, string for PCAN, e.g. PCAN_USBBUS1)")
    ap.add_argument("--bitrate", type=int, default=500000, help="CAN bitrate")
    return ap


def main():
    args = build_arg_parser().parse_args()
    node_id = args.node_id
    bustype = args.bustype_opt if args.bustype_opt is not None else (args.bustype or "ixxat")
    channel = resolve_channel(bustype, args.channel)
    bitrate = args.bitrate

    r = ObjReader(node_id, bustype, channel, bitrate)
    try:
        print(f"=== SDO device object read: node=0x{node_id:02X}, bustype={bustype}, channel={channel}, bitrate={bitrate} ===")
        print(f"{'0x2008 Stored Device Name':<40} = '{r.read_str(0x2008)}'")
        print(f"{'0x2009 Stored HW Version (E-state)':<40} = '{r.read_str(0x2009)}'")
        print(f"{'0x200A Stored SW version':<40} = '{r.read_str(0x200A)}'")
        print(f"{'0x1008 Manufacturer Device Name':<40} = '{r.read_str(0x1008)}'")
        print(f"{'0x1009 Manufacturer HW Version (E-state PCBA)':<40} = '{r.read_str(0x1009)}'")
        print(f"{'0x100A SW version':<40} = '{r.read_str(0x100A)}'")
        stored_pcba_part = r.read_u32(PCBA_PART_NUMBER_INDEX, 0x00)
        print(f"{'0x201A Part number PCBA':<40} = 0x{stored_pcba_part:08X} ({stored_pcba_part})")

        vendor_id = r.read_u32(0x1018, 0x01)
        product_code = r.read_u32(0x1018, 0x02)
        revision_value = r.read_u32(0x1018, 0x03)
        serial_pcba = r.read_u32(0x1018, 0x04)
        revision_pcba, revision_component = decode_revision(revision_value)

        print(f"{'0x1018:01 Vendor ID':<40} = 0x{vendor_id:08X} ({vendor_id})")
        print(f"{'0x1018:02 Part number component':<40} = 0x{product_code:08X} ({product_code})")
        print(f"{'0x1018:03 Revision PCBA':<40} = 0x{revision_pcba:04X} ({revision_pcba})")
        print(f"{'0x1018:03 Revision component':<40} = 0x{revision_component:04X} ({revision_component})")
        print(f"{'0x1018:04 Serial number PCBA':<40} = 0x{serial_pcba:08X} ({serial_pcba})")

        serial_component = r.read_u32(COMPONENT_SERIAL_INDEX, 0x01)
        print(f"{'0x2019:01 Serial number component':<40} = 0x{serial_component:08X} ({serial_component})")
    finally:
        r.close()


if __name__ == "__main__":
    main()