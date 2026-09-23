import time
import canopen
import os


def switch_to_boot(node):
    node.sdo[0x1F51][1].raw = 0


def switch_to_app(node):
    node.sdo[0x1F51][1].raw = 1


if __name__ == "__main__":
    APP_BIN = "../../build/application/application.bin"
    EDS_FILE = "../../src/bootloader/obj_dic/bootloader.eds"
    NODE_ID = 0x34
    BITRATE = 500000

    network = canopen.Network()
    node = canopen.RemoteNode(NODE_ID, EDS_FILE)
    network.add_node(node)
    network.connect(bustype="ixxat", channel=0, bitrate=BITRATE)

    node.sdo.RESPONSE_TIMEOUT = 1
    node.sdo.MAX_RETRIES = 5

    node.nmt.send_command(128)

    if "Application" in node.sdo[0x100A].raw:
        print("Application is active, switching to bootloader")
        switch_to_boot(node)
        time.sleep(2)

    print("Value = ", node.sdo[0x100A].raw)
    filesize = os.path.getsize(APP_BIN)

    with open(APP_BIN, 'rb') as infile, node.sdo[0x1F50][1].open('wb', size=filesize, block_transfer=False) as outfile:
        while True:
            data = infile.read(256)
            print('#', end='', flush=True)
            if not data:
                break
            outfile.write(data)

    print("\nUpdate done, switch to App")
    switch_to_app(node)
    time.sleep(1)
    network.disconnect()