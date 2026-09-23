import argparse
import contextlib
import io
import os
import time

import canopen


PROG_COMMAND_BOOTLOADER = 0
PROG_COMMAND_APPLICATION = 1


def parse_int(value: str) -> int:
    return int(value, 0)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Switch between bootloader and application and verify state")
    parser.add_argument("--node-id", type=parse_int, default=0x30, help="CANopen node id (default: 0x30)")
    parser.add_argument("--channel", type=int, default=0, help="IXXAT channel")
    parser.add_argument("--bitrate", type=int, default=500000, help="CAN bitrate")
    parser.add_argument(
        "--eds",
        default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "bootloader.eds"),
        help="EDS path used for 0x1F51/0x100A access",
    )
    parser.add_argument(
        "--sequence",
        choices=("boot-app", "app-boot", "cycle"),
        default="cycle",
        help="Switch sequence to execute",
    )
    parser.add_argument("--switch-wait", type=float, default=2.0, help="Delay after issuing switch command")
    parser.add_argument("--connect-timeout", type=float, default=8.0, help="Timeout to verify target state")
    parser.add_argument("--sdo-timeout", type=float, default=1.5, help="SDO response timeout")
    parser.add_argument("--sdo-retries", type=int, default=5, help="SDO max retries")
    return parser.parse_args()


def classify_sw_version(sw_version: str) -> str:
    if "bootloader" in sw_version.lower():
        return "bootloader"
    if "application" in sw_version.lower():
        return "application"
    return "unknown"


def open_network(node_id: int, eds_path: str, channel: int, bitrate: int, sdo_timeout: float, sdo_retries: int):
    network = canopen.Network()
    node = canopen.RemoteNode(node_id, eds_path)
    network.add_node(node)
    network.connect(bustype="ixxat", channel=channel, bitrate=bitrate)
    node.sdo.RESPONSE_TIMEOUT = sdo_timeout
    node.sdo.MAX_RETRIES = sdo_retries
    return network, node


def read_sw_version(node) -> str:
    raw = node.sdo[0x100A].raw
    if isinstance(raw, bytes):
        return raw.decode("ascii", errors="ignore").rstrip("\x00")
    return str(raw)


def summarize_exception(exc: Exception) -> str:
    text = str(exc).strip()
    if not text:
        return exc.__class__.__name__
    if "Error warning limit exceeded" in text:
        return "transport not ready"
    if "No SDO response received" in text:
        return "device not responding yet"
    if "Unknown SDO command specified" in text:
        return "device still restarting"
    return text


def quiet_disconnect(network: canopen.Network) -> None:
    with contextlib.suppress(Exception):
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            network.disconnect()


def connect_and_identify(args: argparse.Namespace) -> tuple[canopen.Network, canopen.RemoteNode, str, str]:
    network, node = open_network(
        args.node_id,
        args.eds,
        args.channel,
        args.bitrate,
        args.sdo_timeout,
        args.sdo_retries,
    )
    try:
        try:
            node.nmt.send_command(1)
        except Exception:
            pass
        sw_version = read_sw_version(node)
        state = classify_sw_version(sw_version)
        print(f"[INFO] Connected to node 0x{args.node_id:02X}")
        print(f"[INFO] 0x100A = '{sw_version}'")
        print(f"[INFO] Detected state: {state}")
        return network, node, sw_version, state
    except Exception:
        quiet_disconnect(network)
        raise


def wait_for_target_state(args: argparse.Namespace, expected_state: str) -> tuple[str, str]:
    deadline = time.time() + args.connect_timeout
    last_error = None

    while time.time() < deadline:
        network = None
        try:
            with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
                network, node, sw_version, state = connect_and_identify(args)
            if state == expected_state:
                print(f"[INFO] Connected to node 0x{args.node_id:02X}")
                print(f"[INFO] 0x100A = '{sw_version}'")
                print(f"[INFO] Detected state: {state}")
                print(f"[PASS] Reached {expected_state}: '{sw_version}'")
                quiet_disconnect(network)
                return sw_version, state
            print(f"[INFO] State not yet switched, still '{state}'")
        except Exception as exc:
            last_error = exc
            print(f"[INFO] Reconnect pending: {summarize_exception(exc)}")
        finally:
            if network is not None:
                quiet_disconnect(network)
        time.sleep(0.5)

    if last_error is not None:
        raise RuntimeError(f"Timed out waiting for {expected_state}: {last_error}")
    raise RuntimeError(f"Timed out waiting for {expected_state}")


def issue_switch_command(node, target_state: str) -> None:
    if target_state == "bootloader":
        node.sdo[0x1F51][1].raw = PROG_COMMAND_BOOTLOADER
    elif target_state == "application":
        node.sdo[0x1F51][1].raw = PROG_COMMAND_APPLICATION
    else:
        raise ValueError(f"Unsupported target state: {target_state}")


def switch_once(args: argparse.Namespace, target_state: str) -> None:
    network, node, sw_version, current_state = connect_and_identify(args)
    try:
        if current_state == target_state:
            print(f"[INFO] Node already in {target_state}, no switch required")
            return

        print(f"[ACTION] Switch {current_state} -> {target_state}")
        print(f"[INFO] Issuing 0x1F51:01 = {PROG_COMMAND_BOOTLOADER if target_state == 'bootloader' else PROG_COMMAND_APPLICATION}")
        issue_switch_command(node, target_state)
        print(f"[INFO] Switch command accepted while current version is '{sw_version}'")
    finally:
        quiet_disconnect(network)

    print(f"[INFO] Waiting {args.switch_wait:.2f}s for restart")
    time.sleep(args.switch_wait)
    wait_for_target_state(args, target_state)


def main() -> int:
    args = parse_args()

    if not os.path.isfile(args.eds):
        print(f"[ERR] EDS file not found: {args.eds}")
        return 2

    print(
        f"=== Boot/App switch test start: node=0x{args.node_id:02X}, "
        f"channel={args.channel}, bitrate={args.bitrate}, sequence={args.sequence} ==="
    )

    try:
        if args.sequence == "boot-app":
            switch_once(args, "bootloader")
            switch_once(args, "application")
        elif args.sequence == "app-boot":
            switch_once(args, "application")
            switch_once(args, "bootloader")
        else:
            switch_once(args, "bootloader")
            switch_once(args, "application")

        print("=== SUMMARY: PASS ===")
        return 0
    except Exception as exc:
        print(f"[FAIL] {exc}")
        print("=== SUMMARY: FAIL ===")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())