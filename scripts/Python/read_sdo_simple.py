import argparse
import canopen
import time
import json
from pathlib import Path


def parse_int(x) -> int:
    if isinstance(x, int):
        return x
    return int(str(x), 0)


def decode_str(raw) -> str:
    if isinstance(raw, str):
        return raw.rstrip("\x00")
    if isinstance(raw, (bytes, bytearray)):
        try:
            return raw.decode("ascii").rstrip("\x00")
        except UnicodeDecodeError:
            return raw.decode("latin1").rstrip("\x00")
    return str(raw)


def decode_u32(raw) -> int:
    if isinstance(raw, int):
        return raw & 0xFFFFFFFF
    b = raw if isinstance(raw, (bytes, bytearray)) else bytes(raw)
    return int.from_bytes(b, byteorder="little", signed=False) & 0xFFFFFFFF


def decode_u16(raw) -> int:
    if isinstance(raw, int):
        return raw & 0xFFFF
    b = raw if isinstance(raw, (bytes, bytearray)) else bytes(raw)
    return int.from_bytes(b, byteorder="little", signed=False) & 0xFFFF


def decode_i16(raw) -> int:
    if isinstance(raw, int):
        raw &= 0xFFFF
        return raw - 0x10000 if raw & 0x8000 else raw
    b = raw if isinstance(raw, (bytes, bytearray)) else bytes(raw)
    return int.from_bytes(b[:2], byteorder="little", signed=True)


def decode_u8(raw) -> int:
    if isinstance(raw, int):
        return raw & 0xFF
    b = raw if isinstance(raw, (bytes, bytearray)) else bytes(raw)
    return b[0] if len(b) else 0


DECODERS = {
    "str": decode_str,
    "u32": decode_u32,
    "u16": decode_u16,
    "i16": decode_i16,
    "u8": decode_u8,
}


# Default bit labels for 0x6000 status bytes.
BITS_6000 = {
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


BITS_6200 = {
    1: {
        4: "User Interface Output 5",
        3: "User Interface Output 4",
        2: "User Interface Output 3",
        1: "User Interface Output 2",
        0: "User Interface Output 1",
    },
    2: {
        4: "User Interface Output 5",
        3: "User Interface Output 4",
        2: "User Interface Output 3",
        1: "User Interface Output 2",
        0: "User Interface Output 1",
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


class SdoReader:
    def __init__(self, node_id: int, bustype: str, channel: int, bitrate: int):
        self.network = canopen.Network()
        self.node = canopen.RemoteNode(node_id, None)  # 无 EDS
        self.network.add_node(self.node)
        self.network.connect(bustype=bustype, channel=channel, bitrate=bitrate)

        # 推荐：一次性切到 OPERATIONAL
        self.node.nmt.send_command(1)
        time.sleep(0.1)

    def close(self):
        try:
            self.network.disconnect()
        except Exception as e:
            print(f"network.disconnect 出错（可忽略）：{e}")

    def read(self, index: int, sub: int, typ: str):
        raw = self.node.sdo.upload(index, sub)
        decoder = DECODERS.get(typ)
        if decoder is None:
            raise ValueError(f"Unsupported type: {typ}, supported={list(DECODERS.keys())}")
        return decoder(raw)


def parse_obj(spec: str):
    """
    spec: index:sub:type:label
    e.g. 0x1008:0:str:ManufacturerDeviceName
         0x1014:0:u32:EmcyCobId
    """
    parts = spec.split(":")
    if len(parts) < 3:
        raise ValueError("obj spec must be index:sub:type[:label]")
    index = parse_int(parts[0])
    sub = parse_int(parts[1])
    typ = parts[2].lower()
    label = parts[3] if len(parts) >= 4 else f"0x{index:04X}:{sub:02X}"
    return index, sub, typ, label


def parse_target(target: str):
    t = str(target).strip().lower()
    if t in ("0x6000", "6000"):
        return [
            "0x6000:1:u8:DI_Byte1",
            "0x6000:2:u8:DI_Byte2",
            "0x6000:3:u8:DI_Byte3",
            "0x6000:4:u8:DI_Byte4",
        ]
    if t in ("0x6200", "6200"):
        return [
            "0x6200:1:u8:DO_Byte1",
            "0x6200:2:u8:DO_Byte2",
            "0x6200:3:u8:DO_Byte3",
        ]
    raise ValueError(f"Unsupported target '{target}'. Supported: 0x6000, 0x6200")


def parse_expect(spec: str):
    """
    spec: index:sub:value
    e.g. 0x1014:0:0x00000080
    """
    parts = spec.split(":")
    if len(parts) != 3:
        raise ValueError("expect spec must be index:sub:value")
    index = parse_int(parts[0])
    sub = parse_int(parts[1])
    val = parse_int(parts[2])
    return (index, sub), val


def format_u8_bits(val: int) -> str:
    v = int(val) & 0xFF
    return " ".join(f"b{i}={(v >> i) & 0x1}" for i in range(7, -1, -1))


def format_named_bits_lines(val: int, bit_names: dict[int, str]) -> list[str]:
    v = int(val) & 0xFF
    lines = []
    for bit in sorted(bit_names.keys(), reverse=True):
        bit_val = (v >> bit) & 0x1
        lines.append(f"Bit{bit:<1}  {bit_names[bit]:<35} = {bit_val}")
    return lines


def format_u8_bits_lines(val: int) -> list[str]:
    v = int(val) & 0xFF
    return [f"b{bit}  = {(v >> bit) & 0x1}" for bit in range(7, -1, -1)]


def get_default_bit_map(index: int, sub: int):
    if index == 0x6000:
        return BITS_6000.get(sub)
    if index == 0x6200:
        return BITS_6200.get(sub)
    return None


def load_config(path: str):
    p = Path(path)
    data = json.loads(p.read_text(encoding="utf-8"))
    if not isinstance(data, list):
        raise ValueError("JSON config must be a list")
    return data


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--node-id", type=parse_int, default=0x30)
    ap.add_argument("--bustype", default="ixxat")
    ap.add_argument("--channel", type=int, default=0)
    ap.add_argument("--bitrate", type=int, default=500000)

    ap.add_argument("--config", help="JSON file containing SDO read tests")

    ap.add_argument(
        "--obj",
        action="append",
        default=[],
        help="Object spec index:sub:type[:label], type in {str,u32,u16,u8}. Can be repeated.",
    )
    ap.add_argument(
        "--expect",
        action="append",
        default=[],
        help="Expected value spec index:sub:value. Can be repeated.",
    )
    ap.add_argument(
        "targets",
        nargs="*",
        help="Optional shorthand targets, e.g. 0x6000 or 0x6200",
    )
    ap.add_argument(
        "--show-bits",
        action="store_true",
        help="Show per-bit values for u8 objects.",
    )
    ap.add_argument(
        "--bit-sub",
        action="append",
        default=[],
        help="Sub-index filter for --show-bits, e.g. --bit-sub 2 --bit-sub 3",
    )

    a = ap.parse_args()
    bit_subs = {parse_int(x) for x in a.bit_sub}

    expects = dict(parse_expect(x) for x in a.expect)

    # 从 JSON 载入测试项（优先）
    tests = []
    if a.config:
        for item in load_config(a.config):
            index = parse_int(item["index"])
            sub = parse_int(item.get("sub", 0))
            typ = str(item["type"]).lower()
            label = item.get("label", f"0x{index:04X}:{sub:02X}")
            tests.append((index, sub, typ, label))

            # JSON 内可直接带 expect
            if "expect" in item and typ != "str":
                expects[(index, sub)] = parse_int(item["expect"])

    # 没有 config 就走命令行 --obj / positional targets
    if not tests:
        for t in a.targets:
            a.obj.extend(parse_target(t))

        if not a.obj:
            a.obj = [
                "0x1008:0:str:ManufacturerDeviceName",
                "0x1014:0:u32:EmcyCobId",
            ]
        tests = [parse_obj(s) for s in a.obj]

    r = SdoReader(a.node_id, a.bustype, a.channel, a.bitrate)
    failed = 0
    try:
        print(f"=== SDO READ node=0x{a.node_id:02X} bustype={a.bustype} channel={a.channel} bitrate={a.bitrate} ===")
        if a.config:
            print(f"Config: {a.config}")

        for index, sub, typ, label in tests:
            key = (index, sub)
            try:
                val = r.read(index, sub, typ)
                bit_map = get_default_bit_map(index, sub) if typ == "u8" else None
                if typ == "str":
                    disp = f"'{val}'"
                else:
                    disp = f"0x{int(val):X} ({int(val)})"

                if key in expects and typ != "str":
                    exp = expects[key]
                    ok = int(val) == int(exp)
                    status = "PASS" if ok else "FAIL"
                    if not ok:
                        failed += 1
                    print(f"[{status}] {label:<28} 0x{index:04X}:{sub:02X} {typ:<3} = {disp}, expect=0x{exp:X}")
                else:
                    if bit_map is None:
                        print(f"[INFO] {label:<28} 0x{index:04X}:{sub:02X} {typ:<3} = {disp}")

                # 0x6000/0x6200 status definitions are printed by default.
                if bit_map is not None:
                    print(f"       map  {label:<28} 0x{index:04X}:{sub:02X}")
                    for line in format_named_bits_lines(val, bit_map):
                        print(f"            {line}")

                if typ == "u8" and a.show_bits and (not bit_subs or sub in bit_subs):
                    print(f"       bits {label:<28} 0x{index:04X}:{sub:02X}")
                    for line in format_u8_bits_lines(val):
                        print(f"            {line}")

            except Exception as e:
                failed += 1
                print(f"[FAIL] {label:<28} 0x{index:04X}:{sub:02X} {typ:<3} read error: {e}")

    finally:
        r.close()

    raise SystemExit(1 if failed else 0)


if __name__ == "__main__":
    main()