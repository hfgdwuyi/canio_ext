import time
import os
import canopen

# ====================== 配置区 ======================
APP_BIN = "../../build/application/application.bin"
EDS_FILE = "../../src/bootloader/obj_dic/bootloader.eds"
NODE_ID = 48        # 0x30
BITRATE = 500000
BUS_TYPE = "ixxat"
CHANNEL = 0

TEST_ROUNDS = 10
WAIT_AFTER_ENTER_OP = 3.0
WAIT_AFTER_SWITCH_BOOT = 2.0
WAIT_AFTER_SWITCH_APP = 0.2
NODE_HARDWARE_REBOOT_WAIT = 5
AFTER_DISCONNECT_COOLDOWN = 2.0
STACK_INIT_TIMEOUT = 20.0   # 最大允许等待时间
SDO_TIMEOUT = 1.5
SDO_RETRIES = 5
# ====================================================

def wait_preop_ready(node, timeout_s: float):
    """
    等待节点退出Initialization状态
    捕获0x05040001(Initialization拒绝SDO)、通信超时，持续重试
    返回：成功读到0x100A的版本字符串
    """
    start = time.time()
    while time.time() - start < timeout_s:
        node.nmt.send_command(128)
        time.sleep(0.4)
        try:
            ver = node.sdo[0x100A].raw
            print(f"✅ Node exit Initialization, SDO ready, version:{ver}")
            return ver
        except canopen.SdoAbortedError as e:
            if e.code == 0x05040001:
                print(f"[skip] Abort 0x05040001, still in Initialization")
                continue
            raise
        except canopen.SdoCommunicationError:
            print("[skip] SDO no response")
            continue
    raise OSError(f"Timeout: node not ready after {timeout_s}s")


def step_enter_operational():
    print("\n------ Step: enter operational (sim enter_operational.py) ------")
    network = canopen.Network()
    node = canopen.RemoteNode(NODE_ID, EDS_FILE)
    network.add_node(node)
    try:
        network.connect(bustype=BUS_TYPE, channel=CHANNEL, bitrate=BITRATE)
        node.sdo.RESPONSE_TIMEOUT = SDO_TIMEOUT
        node.sdo.MAX_RETRIES = SDO_RETRIES

        ver = wait_preop_ready(node, STACK_INIT_TIMEOUT)

        # SDO就绪后，再切Operational
        node.nmt.send_command(1)
        time.sleep(0.4)
        ver = node.sdo[0x100A].raw
        print(f"Enter Operational OK, 0x100A: {ver}")
        return ver
    finally:
        network.disconnect()
        print("close enter‑operational session")
        time.sleep(AFTER_DISCONNECT_COOLDOWN)


def step_do_firmware_update():
    print("\n------ Step: run firmware update (sim TC_CANOpen_update.py) ------")
    network = canopen.Network()
    node = canopen.RemoteNode(NODE_ID, EDS_FILE)
    network.add_node(node)
    try:
        network.connect(bustype=BUS_TYPE, channel=CHANNEL, bitrate=BITRATE)
        node.sdo.RESPONSE_TIMEOUT = SDO_TIMEOUT
        node.sdo.MAX_RETRIES = SDO_RETRIES

        dev_version = node.sdo[0x100A].raw
        if "Application" in dev_version:
            print("Application is active, switching to bootloader")
            node.sdo[0x1F51][1].raw = 0
            time.sleep(WAIT_AFTER_SWITCH_BOOT)

        dev_version = node.sdo[0x100A].raw
        print(f"Value = {dev_version}")

        filesize = os.path.getsize(APP_BIN)
        print(f"Bin file size: {filesize} bytes")

        with open(APP_BIN, 'rb') as infile, node.sdo[0x1f50][1].open(
            'wb', size=filesize, block_transfer=False
        ) as outfile:
            while True:
                data = infile.read(256)
                print('#', end='', flush=True)
                if not data:
                    break
                outfile.write(data)

        print("\nDownload complete, send switch‑to‑App (trigger HW reset)")
        node.sdo[0x1F51][1].raw = 1
        time.sleep(WAIT_AFTER_SWITCH_APP)
        return True
    finally:
        network.disconnect()
        print("close update session")
        time.sleep(AFTER_DISCONNECT_COOLDOWN)


def main():
    fail_count = 0
    success_count = 0

    for round_idx in range(1, TEST_ROUNDS + 1):
        print(f"\n\n########## Pressure Test Round {round_idx}/{TEST_ROUNDS} ##########")
        try:
            step_enter_operational()

            print(f"Sleep {WAIT_AFTER_ENTER_OP}s after enter operational ...")
            time.sleep(WAIT_AFTER_ENTER_OP)

            step_do_firmware_update()

            print(f"\nWait hardware reboot {NODE_HARDWARE_REBOOT_WAIT}s ...")
            time.sleep(NODE_HARDWARE_REBOOT_WAIT)

            print("\n------ Step: post‑reboot enter operational and verify ------")
            dev_ver = step_enter_operational()

            if "Application" in dev_ver:
                print(f"✅ Round {round_idx} SUCCESS, verified version:{dev_ver}")
                success_count += 1
            else:
                raise RuntimeError(f"Not Application, version:{dev_ver}")

        except Exception as e:
            fail_count += 1
            print(f"\n❌ Round {round_idx} FAILED: {repr(e)}")
            raise

    print(f"\n\n================ Test Summary ================")
    print(f"Total rounds: {TEST_ROUNDS}, Success:{success_count}, Fail:{fail_count}")


if __name__ == "__main__":
    main()