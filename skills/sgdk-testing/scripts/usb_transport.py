"""Self-contained identity/FIFO subset of the published EverDrive USB protocol.

Derived from this collection's project transport. No reset, SD or save commands.
"""
import argparse
import contextlib
import math
import os
import struct
import time
BAUDRATE=921600
PROTOCOL_ID=5
DEVICE_ID=0x18
CMD_STATUS=0x10
CMD_STATUS2=0x40
CMD_SYSTEM_INFO=0x26
CMD_MEM_WRITE=0x1a
FIFO_ADDRESS=0x1810000
IO_TIMEOUT=2.0

class ProtocolError(RuntimeError):
    """The cartridge did not complete an expected protocol operation."""


def positive_seconds(value):
    seconds = float(value)
    if not math.isfinite(seconds) or seconds <= 0:
        raise argparse.ArgumentTypeError("timeout must be a positive finite number")
    return seconds


class USBTransport:
    """Pro identity and bounded FIFO I/O with one overall deadline."""

    def __init__(self, serial_port, timeout=120.0):
        if not math.isfinite(timeout) or timeout <= 0:
            raise ValueError("timeout must be a positive finite number")
        self.port = serial_port
        self.deadline = time.monotonic() + timeout
        self.identified = False

    def _deadline(self, timeout=IO_TIMEOUT, cleanup=False):
        end = time.monotonic() + timeout
        return end if cleanup else min(end, self.deadline)

    @staticmethod
    def _remaining(end, operation):
        remaining = end - time.monotonic()
        if remaining <= 0:
            raise ProtocolError(f"timeout during {operation}")
        return remaining

    def _write(self, data, operation, cleanup=False):
        end = self._deadline(cleanup=cleanup)
        offset = 0
        while offset < len(data):
            self.port.write_timeout = self._remaining(end, operation)
            size = min(4096, len(data) - offset)
            # Official Link.TxData avoids a 512-byte USB write; retain that
            # compatibility detail without changing the 1024-byte ACK boundary.
            if size == 512:
                size = 256
            written = self.port.write(data[offset:offset + size])
            if not written or written > size:
                raise ProtocolError(f"incomplete serial write during {operation}")
            offset += written

    def _read(self, size, operation, timeout=IO_TIMEOUT):
        end = self._deadline(timeout)
        data = bytearray()
        while len(data) < size:
            self.port.timeout = self._remaining(end, operation)
            block = self.port.read(size - len(data))
            if not block:
                raise ProtocolError(f"timeout waiting for {operation}")
            data.extend(block)
        return bytes(data)

    def _command(self, command, payload=b"", cleanup=False):
        self._write(bytes((0x2B, 0xD4, command, command ^ 0xFF)) + payload,
                    f"command 0x{command:02X}", cleanup)

    def identify(self):
        # Official connection preamble, followed by Gen2/legacy discovery.
        self._write(bytes(66), "connection preamble")
        self.port.reset_input_buffer()
        self._command(CMD_STATUS2)
        self._command(CMD_STATUS)
        head = self._read(2, "device identity")
        if head[0] == 0xA5:
            # Early Mega firmware ignores STATUS2. Its SYS_INF reply still
            # includes the explicit device ID (Mega DeviceIO.getSysInf).
            self._command(CMD_SYSTEM_INFO)
            info = self._read(64, "legacy cartridge system information")
            if info[50] != DEVICE_ID:
                raise ProtocolError(f"expected Mega EverDrive Pro device 0x18; "
                                    f"legacy system information reports 0x{info[50]:02X}")
            self.identified = True
            return {"device": "Mega EverDrive Pro", "protocol_id": PROTOCOL_ID,
                    "device_id": DEVICE_ID, "status_format": "legacy",
                    "mcu_version": f"0x{struct.unpack_from('>H', info, 44)[0]:04X}",
                    "hardware_version": f"0x{struct.unpack_from('>H', info, 46)[0]:04X}",
                    "boot_version": f"0x{struct.unpack_from('>H', info, 48)[0]:04X}"}
        if head[1] == 0xA5:
            raise ProtocolError("legacy N8 status detected; expected Mega EverDrive Pro")
        if head[0] != 0x5A:
            raise ProtocolError(f"invalid device identity: {head.hex()}")
        tail = self._read(2, "device identity")
        if (head[1], tail[0]) != (PROTOCOL_ID, DEVICE_ID):
            raise ProtocolError(f"expected Mega EverDrive Pro protocol 0x05/device 0x18; "
                                f"got 0x{head[1]:02X}/0x{tail[0]:02X}")
        # Mega's Gen2 CMD_STATUS is still the two-byte A5/status response.
        # Discovery consumes that response; do not leave it for the next ACK.
        status = self._read(2, "legacy status response")
        if status[0] != 0xA5:
            raise ProtocolError(f"unsupported Mega status format: {status.hex()}")
        self.identified = True
        return {"device": "Mega EverDrive Pro", "protocol_id": PROTOCOL_ID,
                "device_id": DEVICE_ID, "status_format": "extended"}

    def _require_identity(self):
        if not self.identified:
            raise ProtocolError("identify the cartridge before issuing commands")

    def _status(self, operation, allowed=(0,)):
        self._command(CMD_STATUS)
        response = self._read(2, f"{operation} status")
        if response[0] != 0xA5:
            raise ProtocolError(f"invalid {operation} status: {response.hex()}")
        if response[1] not in allowed:
            raise ProtocolError(f"{operation} failed: cartridge status 0x{response[1]:02X}")

    def _memory_write(self, address, data):
        self._command(CMD_MEM_WRITE, struct.pack(">IIB", address, len(data), 0) + data)

    def _fifo(self, data):
        self._memory_write(FIFO_ADDRESS, data)


@contextlib.contextmanager
def open_serial(port):
    try:
        import serial
    except ImportError as error:
        raise ProtocolError("pySerial is required; install scripts/requirements-usb.txt "
                            "in a host Python virtual environment") from error
    # Match the official tool's disabled modem control; configure before open.
    connection = serial.Serial(port=None, baudrate=BAUDRATE, timeout=IO_TIMEOUT,
                               write_timeout=IO_TIMEOUT,
                               exclusive=True if os.name == "posix" else None)
    try:
        connection.dtr = False
        connection.rts = False
        connection.port = port
        connection.open()
        yield connection
    finally:
        connection.close()
