from __future__ import annotations

import time
import tkinter as tk
from tkinter import messagebox, ttk

from write_mfd_data import (
    ACTIVE_DEVICE_NAME_INDEX,
    ACTIVE_HW_VERSION_INDEX,
    ACTIVE_SW_VERSION_INDEX,
    COMPONENT_SERIAL_INDEX,
    IDENTITY_INDEX,
    PCBA_PART_NUMBER_INDEX,
    SAVE_ASSET_SUBINDEX,
    SAVE_INDEX,
    SAVE_SIGNATURE,
    WRITE_DEVICE_NAME_INDEX,
    WRITE_HW_VERSION_INDEX,
    WRITE_SW_VERSION_INDEX,
    WRITE_IDENTITY_INDEX,
    connect_network,
    decode_revision,
    encode_revision,
    parse_int,
    read_u32,
    safe_disconnect,
    send_nmt_reset_node,
    write_u32,
)

import canopen


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


class MfdHostTool:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("CANopen MFD Host Tool")
        self.root.geometry("1040x760")

        self.network: canopen.Network | None = None
        self.node = None
        self.connected_channel = ""

        self.bustype_var = tk.StringVar(value="ixxat")
        self.channel_var = tk.StringVar(value="")
        self.bitrate_var = tk.StringVar(value="500000")
        self.node_id_var = tk.StringVar(value="0x34")
        self.sdo_timeout_var = tk.StringVar(value="2.0")
        self.sdo_retries_var = tk.StringVar(value="5")
        self.reset_wait_var = tk.StringVar(value="2.0")
        self.status_var = tk.StringVar(value="Disconnected")

        self.stored_vars = {
            "device_name": tk.StringVar(),
            "hw_version": tk.StringVar(),
            "sw_version": tk.StringVar(),
            "part_number_pcba": tk.StringVar(),
            "part_number_component": tk.StringVar(),
            "revision_pcba": tk.StringVar(),
            "revision_component": tk.StringVar(),
            "serial_number_pcba": tk.StringVar(),
            "serial_number_component": tk.StringVar(),
        }
        self.active_vars = {
            "device_name": tk.StringVar(),
            "hw_version": tk.StringVar(),
            "sw_version": tk.StringVar(),
            "part_number_pcba": tk.StringVar(),
            "vendor_id": tk.StringVar(),
            "part_number_component": tk.StringVar(),
            "revision_pcba": tk.StringVar(),
            "revision_component": tk.StringVar(),
            "serial_number_pcba": tk.StringVar(),
            "serial_number_component": tk.StringVar(),
        }

        self._build_ui()

    def _build_ui(self) -> None:
        root_frame = ttk.Frame(self.root, padding=12)
        root_frame.pack(fill=tk.BOTH, expand=True)

        conn_frame = ttk.LabelFrame(root_frame, text="Connection", padding=10)
        conn_frame.pack(fill=tk.X)

        ttk.Label(conn_frame, text="Backend").grid(row=0, column=0, sticky=tk.W, padx=4, pady=4)
        ttk.Combobox(conn_frame, textvariable=self.bustype_var, values=("ixxat", "pcan"), state="readonly", width=12).grid(row=0, column=1, sticky=tk.W, padx=4, pady=4)
        ttk.Label(conn_frame, text="Channel").grid(row=0, column=2, sticky=tk.W, padx=4, pady=4)
        ttk.Entry(conn_frame, textvariable=self.channel_var, width=16).grid(row=0, column=3, sticky=tk.W, padx=4, pady=4)
        ttk.Label(conn_frame, text="Bitrate").grid(row=0, column=4, sticky=tk.W, padx=4, pady=4)
        ttk.Entry(conn_frame, textvariable=self.bitrate_var, width=12).grid(row=0, column=5, sticky=tk.W, padx=4, pady=4)
        ttk.Label(conn_frame, text="Node ID").grid(row=0, column=6, sticky=tk.W, padx=4, pady=4)
        ttk.Entry(conn_frame, textvariable=self.node_id_var, width=10).grid(row=0, column=7, sticky=tk.W, padx=4, pady=4)

        ttk.Label(conn_frame, text="SDO Timeout").grid(row=1, column=0, sticky=tk.W, padx=4, pady=4)
        ttk.Entry(conn_frame, textvariable=self.sdo_timeout_var, width=12).grid(row=1, column=1, sticky=tk.W, padx=4, pady=4)
        ttk.Label(conn_frame, text="SDO Retries").grid(row=1, column=2, sticky=tk.W, padx=4, pady=4)
        ttk.Entry(conn_frame, textvariable=self.sdo_retries_var, width=12).grid(row=1, column=3, sticky=tk.W, padx=4, pady=4)
        ttk.Label(conn_frame, text="Reset Wait").grid(row=1, column=4, sticky=tk.W, padx=4, pady=4)
        ttk.Entry(conn_frame, textvariable=self.reset_wait_var, width=12).grid(row=1, column=5, sticky=tk.W, padx=4, pady=4)
        ttk.Button(conn_frame, text="Connect", command=self.connect).grid(row=1, column=6, sticky=tk.EW, padx=4, pady=4)
        ttk.Button(conn_frame, text="Disconnect", command=self.disconnect).grid(row=1, column=7, sticky=tk.EW, padx=4, pady=4)

        status_frame = ttk.Frame(root_frame)
        status_frame.pack(fill=tk.X, pady=(8, 0))
        ttk.Label(status_frame, text="Status:").pack(side=tk.LEFT)
        ttk.Label(status_frame, textvariable=self.status_var).pack(side=tk.LEFT, padx=(6, 0))

        content_frame = ttk.Frame(root_frame)
        content_frame.pack(fill=tk.BOTH, expand=True, pady=(10, 0))
        content_frame.columnconfigure(0, weight=1)
        content_frame.columnconfigure(1, weight=1)

        self._build_stored_frame(content_frame)
        self._build_active_frame(content_frame)

        button_frame = ttk.Frame(root_frame)
        button_frame.pack(fill=tk.X, pady=(10, 0))
        ttk.Button(button_frame, text="Read All", command=self.read_all).pack(side=tk.LEFT, padx=4)
        ttk.Button(button_frame, text="Write RAM Only", command=lambda: self.write_all(save=False, reset=False)).pack(side=tk.LEFT, padx=4)
        ttk.Button(button_frame, text="Write + Save EEPROM", command=lambda: self.write_all(save=True, reset=False)).pack(side=tk.LEFT, padx=4)
        ttk.Button(button_frame, text="Write + Save + Reset", command=lambda: self.write_all(save=True, reset=True)).pack(side=tk.LEFT, padx=4)
        ttk.Button(button_frame, text="Save Current Stored", command=self.save_current_stored).pack(side=tk.LEFT, padx=4)
        ttk.Button(button_frame, text="Reset Node", command=self.reset_node).pack(side=tk.LEFT, padx=4)

        log_frame = ttk.LabelFrame(root_frame, text="Log", padding=8)
        log_frame.pack(fill=tk.BOTH, expand=True, pady=(10, 0))
        self.log_widget = tk.Text(log_frame, height=14, wrap=tk.WORD)
        self.log_widget.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scroll = ttk.Scrollbar(log_frame, orient=tk.VERTICAL, command=self.log_widget.yview)
        scroll.pack(side=tk.RIGHT, fill=tk.Y)
        self.log_widget.configure(yscrollcommand=scroll.set)

    def _build_stored_frame(self, parent: ttk.Frame) -> None:
        frame = ttk.LabelFrame(parent, text="Stored MFD (Writable / Saveable)", padding=10)
        frame.grid(row=0, column=0, sticky="nsew", padx=(0, 6))
        parent.rowconfigure(0, weight=1)

        fields = [
            ("Device Name (0x2008)", "device_name"),
            ("HW Version / E-state (0x2009)", "hw_version"),
            ("SW Version (0x200A)", "sw_version"),
            ("Part Number PCBA (0x201A)", "part_number_pcba"),
            ("Part Number Component (0x2018:02)", "part_number_component"),
            ("Revision PCBA (0x2018:03 hi16)", "revision_pcba"),
            ("Revision Component (0x2018:03 lo16)", "revision_component"),
            ("Serial Number PCBA (0x2018:04)", "serial_number_pcba"),
            ("Serial Number Component (0x2019:01)", "serial_number_component"),
        ]
        for row, (label, key) in enumerate(fields):
            ttk.Label(frame, text=label).grid(row=row, column=0, sticky=tk.W, padx=4, pady=5)
            ttk.Entry(frame, textvariable=self.stored_vars[key], width=34).grid(row=row, column=1, sticky=tk.EW, padx=4, pady=5)
        frame.columnconfigure(1, weight=1)

    def _build_active_frame(self, parent: ttk.Frame) -> None:
        frame = ttk.LabelFrame(parent, text="Active MFD (Readback)", padding=10)
        frame.grid(row=0, column=1, sticky="nsew", padx=(6, 0))

        fields = [
            ("Device Name (0x1008)", "device_name"),
            ("HW Version (0x1009)", "hw_version"),
            ("SW Version (0x100A)", "sw_version"),
            ("Part Number PCBA (0x201A direct)", "part_number_pcba"),
            ("Vendor ID (0x1018:01)", "vendor_id"),
            ("Part Number Component (0x1018:02)", "part_number_component"),
            ("Revision PCBA (0x1018:03 hi16)", "revision_pcba"),
            ("Revision Component (0x1018:03 lo16)", "revision_component"),
            ("Serial Number PCBA (0x1018:04)", "serial_number_pcba"),
            ("Serial Number Component (0x2019:01)", "serial_number_component"),
        ]
        for row, (label, key) in enumerate(fields):
            ttk.Label(frame, text=label).grid(row=row, column=0, sticky=tk.W, padx=4, pady=5)
            entry = ttk.Entry(frame, textvariable=self.active_vars[key], width=34, state="readonly")
            entry.grid(row=row, column=1, sticky=tk.EW, padx=4, pady=5)
        frame.columnconfigure(1, weight=1)

    def log(self, message: str) -> None:
        self.log_widget.insert(tk.END, message + "\n")
        self.log_widget.see(tk.END)

    def _connection_args(self) -> tuple[str, object | None, int, int, float, int]:
        bustype = self.bustype_var.get().strip() or "ixxat"
        channel_text = self.channel_var.get().strip()
        channel = None if channel_text == "" else channel_text
        if bustype == "ixxat" and channel is not None:
            channel = parse_int(channel)
        bitrate = int(self.bitrate_var.get().strip())
        node_id = parse_int(self.node_id_var.get().strip())
        sdo_timeout = float(self.sdo_timeout_var.get().strip())
        sdo_retries = int(self.sdo_retries_var.get().strip())
        return bustype, channel, bitrate, node_id, sdo_timeout, sdo_retries

    def _ensure_connected(self) -> None:
        if self.network is not None and self.node is not None:
            return
        self.connect()
        if self.network is None or self.node is None:
            raise RuntimeError("Connection was not established")

    def connect(self) -> None:
        self.disconnect(silent=True)
        try:
            bustype, channel, bitrate, node_id, sdo_timeout, sdo_retries = self._connection_args()
            self.network = canopen.Network()
            self.connected_channel = str(connect_network(self.network, bustype, channel, bitrate))
            self.node = canopen.RemoteNode(node_id, None)
            self.network.add_node(self.node)
            self.node.sdo.RESPONSE_TIMEOUT = sdo_timeout
            self.node.sdo.MAX_RETRIES = sdo_retries
            self.node.nmt.send_command(128)
            self.status_var.set(f"Connected: {bustype}/{self.connected_channel}, node=0x{node_id:02X}")
            self.log(self.status_var.get())
        except Exception as exc:
            self.disconnect(silent=True)
            self.status_var.set("Disconnected")
            messagebox.showerror("Connect failed", str(exc))

    def disconnect(self, silent: bool = False) -> None:
        if self.network is not None:
            safe_disconnect(self.network)
        self.network = None
        self.node = None
        self.connected_channel = ""
        self.status_var.set("Disconnected")
        if not silent:
            self.log("Disconnected")

    def _read_component_serial(self) -> str:
        assert self.node is not None
        value = read_u32(self.node, COMPONENT_SERIAL_INDEX, 0x01)
        return str(value)

    def read_all(self) -> None:
        try:
            self._ensure_connected()
            assert self.node is not None
            self.stored_vars["device_name"].set(read_vis_string(self.node, WRITE_DEVICE_NAME_INDEX))
            self.stored_vars["hw_version"].set(read_vis_string(self.node, WRITE_HW_VERSION_INDEX))
            self.stored_vars["sw_version"].set(read_vis_string(self.node, WRITE_SW_VERSION_INDEX))
            self.stored_vars["part_number_pcba"].set(str(read_u32(self.node, PCBA_PART_NUMBER_INDEX, 0x00)))
            stored_product = read_u32(self.node, WRITE_IDENTITY_INDEX, 0x02)
            stored_revision = read_u32(self.node, WRITE_IDENTITY_INDEX, 0x03)
            stored_serial = read_u32(self.node, WRITE_IDENTITY_INDEX, 0x04)
            stored_component_serial = read_u32(self.node, COMPONENT_SERIAL_INDEX, 0x01)
            stored_revision_pcba, stored_revision_component = decode_revision(stored_revision)
            self.stored_vars["part_number_component"].set(str(stored_product))
            self.stored_vars["revision_pcba"].set(str(stored_revision_pcba))
            self.stored_vars["revision_component"].set(str(stored_revision_component))
            self.stored_vars["serial_number_pcba"].set(str(stored_serial))
            self.stored_vars["serial_number_component"].set(str(stored_component_serial))

            self.active_vars["device_name"].set(read_vis_string(self.node, ACTIVE_DEVICE_NAME_INDEX))
            self.active_vars["hw_version"].set(read_vis_string(self.node, ACTIVE_HW_VERSION_INDEX))
            self.active_vars["sw_version"].set(read_vis_string(self.node, ACTIVE_SW_VERSION_INDEX))
            self.active_vars["part_number_pcba"].set(str(read_u32(self.node, PCBA_PART_NUMBER_INDEX, 0x00)))
            active_vendor = read_u32(self.node, IDENTITY_INDEX, 0x01)
            active_product = read_u32(self.node, IDENTITY_INDEX, 0x02)
            active_revision = read_u32(self.node, IDENTITY_INDEX, 0x03)
            active_serial = read_u32(self.node, IDENTITY_INDEX, 0x04)
            active_revision_pcba, active_revision_component = decode_revision(active_revision)
            self.active_vars["vendor_id"].set(str(active_vendor))
            self.active_vars["part_number_component"].set(str(active_product))
            self.active_vars["revision_pcba"].set(str(active_revision_pcba))
            self.active_vars["revision_component"].set(str(active_revision_component))
            self.active_vars["serial_number_pcba"].set(str(active_serial))
            self.active_vars["serial_number_component"].set(self._read_component_serial())
            self.log("Read stored and active MFD values.")
        except Exception as exc:
            messagebox.showerror("Read failed", str(exc))

    def _collect_stored_values(self) -> dict[str, int | str]:
        return {
            "device_name": self.stored_vars["device_name"].get().strip(),
            "hw_version": self.stored_vars["hw_version"].get().strip(),
            "sw_version": self.stored_vars["sw_version"].get().strip(),
            "part_number_pcba": parse_int(self.stored_vars["part_number_pcba"].get().strip()),
            "part_number_component": parse_int(self.stored_vars["part_number_component"].get().strip()),
            "revision_pcba": parse_int(self.stored_vars["revision_pcba"].get().strip()),
            "revision_component": parse_int(self.stored_vars["revision_component"].get().strip()),
            "serial_number_pcba": parse_int(self.stored_vars["serial_number_pcba"].get().strip()),
            "serial_number_component": parse_int(self.stored_vars["serial_number_component"].get().strip()),
        }

    def write_all(self, save: bool, reset: bool) -> None:
        try:
            self._ensure_connected()
            assert self.node is not None
            values = self._collect_stored_values()
            write_vis_string(self.node, WRITE_DEVICE_NAME_INDEX, str(values["device_name"]))
            write_vis_string(self.node, WRITE_HW_VERSION_INDEX, str(values["hw_version"]))
            write_vis_string(self.node, WRITE_SW_VERSION_INDEX, str(values["sw_version"]))
            write_u32(self.node, PCBA_PART_NUMBER_INDEX, 0x00, int(values["part_number_pcba"]))
            write_u32(self.node, WRITE_IDENTITY_INDEX, 0x02, int(values["part_number_component"]))
            write_u32(
                self.node,
                WRITE_IDENTITY_INDEX,
                0x03,
                encode_revision(int(values["revision_pcba"]), int(values["revision_component"])),
            )
            write_u32(self.node, WRITE_IDENTITY_INDEX, 0x04, int(values["serial_number_pcba"]))
            write_u32(self.node, COMPONENT_SERIAL_INDEX, 0x01, int(values["serial_number_component"]))
            self.log("Wrote stored MFD values to RAM.")

            if save:
                write_u32(self.node, SAVE_INDEX, SAVE_ASSET_SUBINDEX, SAVE_SIGNATURE)
                self.log("Saved stored MFD values to EEPROM via 0x1010:04.")

            if reset:
                self._reset_node_internal()

            self.read_all()
        except Exception as exc:
            messagebox.showerror("Write failed", str(exc))

    def save_current_stored(self) -> None:
        try:
            self._ensure_connected()
            assert self.node is not None
            write_u32(self.node, SAVE_INDEX, SAVE_ASSET_SUBINDEX, SAVE_SIGNATURE)
            self.log("Saved current stored MFD values to EEPROM via 0x1010:04.")
        except Exception as exc:
            messagebox.showerror("Save failed", str(exc))

    def _reset_node_internal(self) -> None:
        assert self.network is not None
        _, channel, bitrate, node_id, sdo_timeout, sdo_retries = self._connection_args()
        bustype = self.bustype_var.get().strip() or "ixxat"
        reset_wait = float(self.reset_wait_var.get().strip())

        send_nmt_reset_node(self.network, node_id)
        self.log(f"Sent NMT reset node to 0x{node_id:02X}.")
        self.root.update_idletasks()
        time.sleep(reset_wait)
        safe_disconnect(self.network)
        self.network = None
        self.node = None

        self.network = canopen.Network()
        self.connected_channel = str(connect_network(self.network, bustype, channel, bitrate))
        self.node = canopen.RemoteNode(node_id, None)
        self.network.add_node(self.node)
        self.node.sdo.RESPONSE_TIMEOUT = sdo_timeout
        self.node.sdo.MAX_RETRIES = sdo_retries
        self.node.nmt.send_command(128)
        self.status_var.set(f"Connected: {bustype}/{self.connected_channel}, node=0x{node_id:02X}")
        self.log("Reconnected after node reset.")

    def reset_node(self) -> None:
        try:
            self._ensure_connected()
            self._reset_node_internal()
            self.read_all()
        except Exception as exc:
            messagebox.showerror("Reset failed", str(exc))


def main() -> None:
    root = tk.Tk()
    app = MfdHostTool(root)
    root.protocol("WM_DELETE_WINDOW", lambda: (app.disconnect(silent=True), root.destroy()))
    root.mainloop()


if __name__ == "__main__":
    main()