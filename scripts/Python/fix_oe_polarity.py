#!/usr/bin/env python3
"""Diagnose and fix 0x6202:03 output polarity for byte 2.

Background
----------
This script normalizes 0x6202:03 (DO Polarity, Output Byte 2) so that
0x6200:03 bit 4 (UI power supply enable) and bit 5 (UI LED output enable)
are inverted by default, while bit 6 (CAN power supply enable) keeps direct
polarity.

Historically some nodes used older polarity values such as 0x00
(all direct), 0x40 (bit 6 inverted only) or 0x50 (bit 4 + bit 6 inverted).
With
CONFIG_NON_VOLATILE_MEM enabled, a previously saved value may still be
restored from EEPROM at boot.

This script connects to the node, reads the current 0x6202:03 value, diagnoses
whether EEPROM is the cause, and can fix it:

    --mode check    read-only diagnosis
        --mode ram      fix in RAM only (0x6202:03 = 0x30), no EEPROM write
    --mode apply    fix in RAM + persist via 0x1010:01 "save" (default)
        --mode restore  erase parameter EEPROM, reset, then re-apply 0x30 and save

Usage:
    python fix_oe_polarity.py                      # IXXAT, diagnose + apply + save
    python fix_oe_polarity.py pcan                 # PCAN, auto channel detection
    python fix_oe_polarity.py pcan --mode check    # PCAN, diagnose only
    python fix_oe_polarity.py pcan --channel PCAN_USBBUS1 --node-id 0x34
    python fix_oe_polarity.py --eds application.eds
    python fix_oe_polarity.py --mode check --node-id 48 --channel 0
"""
import argparse
import os
import sys
import time

import can
import canopen

# --------------------------------------------------------------------------- #
# Defaults (match the project setup)
# --------------------------------------------------------------------------- #
NODE_ID = 52          # 0x34
BITRATE = 500000
_SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_EDS_NAME = "application.eds"
REPO_EDS = os.path.normpath(os.path.join(_SCRIPT_DIR, "..", "..", "src", "application", "obj_dic", DEFAULT_EDS_NAME))

POLARITY_INDEX = 0x6202
POLARITY_SUB = 3
EXPECTED = 0x30       # bit 4 + bit 5 inverted, bit 6 direct
OLD = 0x50            # bit 4 + bit 6 inverted
OLDER = 0x40          # bit 6 inverted only
OLDEST = 0x00         # all direct

WRITE_INDEX = 0x6200
WRITE_SUB = 3

SAVE_INDEX = 0x1010   # sub-index 1 "Save all Parameters"
LOAD_INDEX = 0x1011   # sub-index 1 "Restore all Default Parameters"
SIG_SAVE = 0x65766173  # "save"
SIG_LOAD = 0x64616F6C  # "load"

NMT_RESET_NODE = 0x81
NMT_PREOP = 0x80


# --------------------------------------------------------------------------- #
# Small helpers
# --------------------------------------------------------------------------- #
def parse_int(x):
    if isinstance(x, int):
        return x
    return int(str(x), 0)


def read_u8(node, index, sub):
    raw = node.sdo.upload(index, sub)
    b = raw if isinstance(raw, (bytes, bytearray)) else bytes(raw)
    return b[0] if b else 0


def write_u8(node, index, sub, value):
    node.sdo.download(index, sub, bytes([value & 0xFF]))


def read_vis_string(node, index, sub=0):
    raw = node.sdo.upload(index, sub)
    data = bytes(raw)
    return data.split(b"\x00", 1)[0].decode("ascii", errors="replace")


def write_u32(node, index, sub, value):
    node.sdo.download(index, sub, int(value & 0xFFFFFFFF).to_bytes(4, "little", signed=False))


def try_store_signature(node, index, sub, value, label):
    try:
        write_u32(node, index, sub, value)
        return True
    except canopen.SdoCommunicationError as exc:
        print(
            f"[WARN] No SDO response after {label} command ({exc}). "
            "Some firmware versions finish the non-volatile operation without replying."
        )
        return False


def resolve_eds_path(eds_arg):
    if eds_arg:
        return os.path.abspath(eds_arg)

    local_eds = os.path.join(_SCRIPT_DIR, DEFAULT_EDS_NAME)
    if os.path.isfile(local_eds):
        return local_eds

    if os.path.isfile(REPO_EDS):
        return REPO_EDS

    return None


def resolve_channel(bustype, channel_value):
    if channel_value is not None:
        return channel_value
    return "PCAN_USBBUS1" if bustype == "pcan" else 0


def connect_ixxat(network, channel, bitrate):
    try:
        network.connect(bustype="ixxat", channel=channel, bitrate=bitrate)
    except TypeError:
        network.connect(bustype="ixxat", channel=channel, bitrate=bitrate)


def connect_pcan(network, channel, bitrate):
    """Connect to PCAN, auto-detecting the channel when none is given."""
    if channel is not None:
        try:
            network.connect(interface="pcan", channel=channel, bitrate=bitrate)
            print(f"PCAN connected on channel: {channel}")
            return
        except TypeError:
            network.connect(bustype="pcan", channel=channel, bitrate=bitrate)
            print(f"PCAN connected on channel: {channel}")
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


def connect_network(network, bustype, channel_value, bitrate):
    if bustype == "pcan":
        connect_pcan(network, channel_value, bitrate)
    else:
        connect_ixxat(network, resolve_channel(bustype, channel_value), bitrate)


def wait_sdo_ready(node, timeout=10.0):
    """Wait until the application answers SDO (i.e. exited Initialization)."""
    start = time.time()
    while time.time() - start < timeout:
        node.nmt.send_command(NMT_PREOP)
        time.sleep(0.4)
        try:
            return read_vis_string(node, 0x100A, 0x00)
        except canopen.SdoAbortedError as exc:
            if exc.code == 0x05040001:
                # still in Initialization, keep retrying
                continue
            raise
    raise RuntimeError("Node did not become SDO-ready in time")


# --------------------------------------------------------------------------- #
# Main
# --------------------------------------------------------------------------- #
def main():
    parser = argparse.ArgumentParser(description="Diagnose/fix 0x6202:03 output polarity for bit 4 and bit 6")
    parser.add_argument("bustype", nargs="?", choices=("ixxat", "pcan"), default=None,
                        help="CAN backend to use: ixxat or pcan (e.g. `python fix_oe_polarity.py pcan`)")
    parser.add_argument("--bustype", dest="bustype_opt", choices=("ixxat", "pcan"), default=None,
                        help="CAN backend to use: ixxat or pcan")
    parser.add_argument("--node-id", type=parse_int, default=NODE_ID)
    parser.add_argument(
        "--eds",
        default=None,
        help=(
            "Optional EDS path. If omitted, the script first looks for "
            "application.eds next to this .py file, then in the repo obj_dic folder."
        ),
    )
    parser.add_argument("--channel", default=None,
                        help="CAN interface channel (int for IXXAT, string for PCAN, e.g. PCAN_USBBUS1)")
    parser.add_argument("--bitrate", type=int, default=BITRATE)
    parser.add_argument("--mode", choices=["check", "ram", "apply", "restore"], default="apply",
                        help="check=diagnose only, ram=fix in RAM, apply=fix+save (default), restore=erase EEPROM+reset")
    args = parser.parse_args()

    bustype = args.bustype_opt if args.bustype_opt is not None else (args.bustype or "ixxat")

    eds_path = resolve_eds_path(args.eds)

    network = canopen.Network()
    node = canopen.RemoteNode(args.node_id, eds_path)
    network.add_node(node)

    print(f"[INFO] Connecting via {bustype}, node {args.node_id} @ {args.bitrate} bps")
    if eds_path is not None:
        print(f"[INFO] Using EDS: {eds_path}")
    else:
        print("[INFO] No EDS found - continuing with raw SDO access only")
    connect_network(network, bustype, args.channel, args.bitrate)
    try:
        try:
            version = wait_sdo_ready(node)
        except RuntimeError as exc:
            print(f"[ERR] {exc}")
            sys.exit(2)
        print(f"[INFO] Node version: {version}")

        if "boot" in version.lower() and "application" not in version.lower():
            print("[WARN] Node reports a bootloader version - 0x6202 may not exist. "
                  "Start the application (0x1F51:1 = 1) and retry.")
            if args.mode == "check":
                return 0

        current = read_u8(node, POLARITY_INDEX, POLARITY_SUB)
        print(f"[INFO] Current 0x{WRITE_INDEX:04X}:03 write = 0x{read_u8(node, WRITE_INDEX, WRITE_SUB):02X}")
        print(f"[INFO] Current 0x{POLARITY_INDEX:04X}:03 polarity = 0x{current:02X} (expected 0x{EXPECTED:02X})")

        if current == EXPECTED:
            print("[OK] Polarity already correct - nothing to fix.")
            return 0
        if current == OLD:
            print(f"[DIAG] 0x{POLARITY_INDEX:04X}:03 = 0x{OLD:02X}: bit 4 and bit 6 are inverted.")
        elif current == OLDER:
            print(f"[DIAG] 0x{POLARITY_INDEX:04X}:03 = 0x{OLDER:02X}: bit 6 is inverted.")
        elif current == OLDEST:
            print(f"[DIAG] 0x{POLARITY_INDEX:04X}:03 = 0x{OLDEST:02X}: all bits use direct polarity.")
        else:
            print(f"[DIAG] Unusual polarity value 0x{current:02X}.")

        if args.mode == "check":
            print("[INFO] Check mode - no changes made.")
            return 0

        # --- ram / apply: force the value now ------------------------------- #
        write_u8(node, POLARITY_INDEX, POLARITY_SUB, EXPECTED)
        time.sleep(0.2)
        print(f"[INFO] Wrote 0x{POLARITY_INDEX:04X}:03 = 0x{EXPECTED:02X} (RAM)")

        if args.mode == "restore":
            # Erase saved parameters first, then re-apply the direct polarity and save it.
            print(f"[INFO] Writing 0x{LOAD_INDEX:04X}:01 = 'load' to erase parameter EEPROM ...")
            try_store_signature(node, LOAD_INDEX, 0x01, SIG_LOAD, "load")
            print("[INFO] Sending NMT Reset Node ...")
            node.nmt.send_command(NMT_RESET_NODE)
            try:
                wait_sdo_ready(node, timeout=10.0)
            except RuntimeError as exc:
                print(f"[ERR] Node did not become ready after reset: {exc}")
                sys.exit(2)
            current = read_u8(node, POLARITY_INDEX, POLARITY_SUB)
            print(f"[INFO] After reset, 0x{POLARITY_INDEX:04X}:03 = 0x{current:02X}")

            write_u8(node, POLARITY_INDEX, POLARITY_SUB, EXPECTED)
            time.sleep(0.2)
            readback = read_u8(node, POLARITY_INDEX, POLARITY_SUB)
            print(f"[INFO] Re-applied direct polarity, readback = 0x{readback:02X}")
            print(f"[INFO] Persisting via 0x{SAVE_INDEX:04X}:01 = 'save' ...")
            saved = try_store_signature(node, SAVE_INDEX, 0x01, SIG_SAVE, "save")
            if saved:
                print("[OK] EEPROM defaults cleared and direct polarity saved.")
            else:
                print("[WARN] Save command was sent but not acknowledged. Power-cycle the node and rerun --mode check to confirm persistence.")
        else:
            readback = read_u8(node, POLARITY_INDEX, POLARITY_SUB)
            print(f"[INFO] Readback 0x{POLARITY_INDEX:04X}:03 = 0x{readback:02X}")

            if args.mode == "apply":
                print(f"[INFO] Persisting via 0x{SAVE_INDEX:04X}:01 = 'save' ...")
                saved = try_store_signature(node, SAVE_INDEX, 0x01, SIG_SAVE, "save")
                if saved:
                    print("[OK] Saved to EEPROM. Direct polarity is now persistent.")
                else:
                    print("[WARN] Save command was sent but not acknowledged. Power-cycle the node and rerun --mode check to confirm persistence.")
            else:
                print("[WARN] RAM-only fix - it is lost on the next boot unless you "
                      "also run with --mode apply (saves) or --mode restore (defaults).")

        print("[OK] Done.")
    finally:
        try:
            network.disconnect()
        except Exception:
            pass


if __name__ == "__main__":
    main()
