#!/usr/bin/env python3
"""Upload and launch a ROM on Mega EverDrive Pro, directly over serial USB.

Protocol sources and hardware acceptance: ../references/everdrive-usb.md.
Requires Python 3.10+ and pySerial. No edlink, Mono, or game-side USB code.
"""
import argparse
import binascii
import contextlib
import hashlib
import json
import math
import os
from pathlib import Path
import secrets
import struct
import sys
import time


BAUDRATE = 921600
BLOCK_SIZE = 1024
PROTOCOL_ID = 0x05
DEVICE_ID = 0x18
CMD_STATUS = 0x10
CMD_STATUS2 = 0x40
CMD_MEM_WRITE = 0x1A
CMD_SYSTEM_INFO = 0x26
CMD_RESET = 0x29
CMD_FILE_OPEN = 0xC9
CMD_FILE_WRITE = 0xCC
CMD_FILE_CLOSE = 0xCE
CMD_MKDIR = 0xD2
CONFIG_ADDRESS = 0x1800000
FIFO_ADDRESS = 0x1810000
LAB_LEASE_SECONDS = 5
LAB_XOR = 0xA55A5AA5
IO_TIMEOUT = 2.0


def lab_packet(kind, nonce):
    body = struct.pack(">4sHI", b"HLV1", kind, nonce)
    return body + struct.pack(">H", binascii.crc_hqx(body, 0xFFFF))


class ProtocolError(RuntimeError):
    """The cartridge did not complete an expected protocol operation."""


def positive_seconds(value):
    seconds = float(value)
    if not math.isfinite(seconds) or seconds <= 0:
        raise argparse.ArgumentTypeError("timeout must be a positive finite number")
    return seconds


def lab_duration(value):
    seconds = float(value)
    if not math.isfinite(seconds) or not 0 <= seconds <= 300:
        raise argparse.ArgumentTypeError("duration must be finite, from 0 to 300 seconds")
    return seconds


class MegaEverDrivePro:
    """Mega Gen1/Gen2 transport with verified Pro identity and ROM installation.

    Reads/writes tolerate fragmentation, have per-operation deadlines, and share
    a deployment deadline. Only reset release can exceed that deadline (by at
    most IO_TIMEOUT) so an interrupted operation can release the console.
    """

    def __init__(self, serial_port, timeout=120.0):
        if not math.isfinite(timeout) or timeout <= 0:
            raise ValueError("timeout must be a positive finite number")
        self.port = serial_port
        self.deadline = time.monotonic() + timeout
        self.identified = False
        self.reset_asserted = False

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

    def _lab_reply(self, kind, nonce):
        # Sliding frame search tolerates stale/fragmented replies and noise.
        end = self._deadline()
        expected = lab_packet(kind, nonce)
        window = bytearray()
        while bytes(window) != expected:
            window.extend(self._read(1, "Hardware Lab handshake", self._remaining(end, "Hardware Lab handshake")))
            if len(window) > len(expected):
                del window[0]

    def verify_lab(self, duration=0.0):
        """Lab-only FIFO challenge/response/ack; no reset, ROM or save writes.

        A five-second guest lease requires periodic fresh exchanges. This is
        cooperative liveness evidence, not authentication or console identity.
        """
        self._require_identity()
        if not math.isfinite(duration) or not 0 <= duration <= 300:
            raise ValueError("duration must be finite, from 0 to 300 seconds")
        started = time.monotonic()
        checks, previous = 0, 0
        while True:
            nonce = secrets.randbelow(0xFFFFFFFF) + 1
            if nonce == previous:
                nonce = nonce % 0xFFFFFFFF + 1
            previous = nonce
            self._fifo(lab_packet(1, nonce))
            self._lab_reply(2, nonce ^ LAB_XOR)
            self._fifo(lab_packet(3, nonce))
            self._lab_reply(4, nonce)
            checks += 1
            elapsed = time.monotonic() - started
            if elapsed >= duration:
                return {"lab_link_verified": True, "handshakes": checks,
                        "lease_seconds": LAB_LEASE_SECONDS,
                        "elapsed_seconds": round(elapsed, 3), "console_model": "unknown"}
            time.sleep(min(1.0, duration - elapsed,
                           self._remaining(self.deadline, "Hardware Lab verification")))

    def _release_reset(self):
        if self.reset_asserted:
            self._command(CMD_RESET, b"\x00", cleanup=True)
            self.reset_asserted = False

    def _reset_to_menu(self, reset):
        self._require_identity()
        self.reset_asserted = True  # Also attempt release after a partial write.
        try:
            self._command(CMD_RESET, bytes((2 if reset == "hard" else 1,)))
            time.sleep(0.060)  # Official reset holds at least 50 ms + 10 ms.
            self._memory_write(CONFIG_ADDRESS, bytes(256))
        except BaseException as original:
            try:
                self._release_reset()
            except (OSError, ProtocolError) as cleanup:
                raise ProtocolError(f"{original}; reset release also failed: {cleanup}") from original
            raise
        self._release_reset()
        ready = self._read(1, "menu ready", 6.0 if reset == "hard" else 2.0)
        if ready != b"r":
            raise ProtocolError(f"unexpected menu response: {ready.hex()}")

    def deploy(self, data, destination, reset="hard"):
        self._require_identity()
        if reset not in ("hard", "soft"):
            raise ValueError("reset must be hard or soft")
        if not data or len(data) > 0xFFFFFFFF:
            raise ValueError("ROM must contain 1 to 4294967295 bytes")
        path = _encode_path(destination)
        if not destination.startswith("usb-games/") or "/" in destination[len("usb-games/"):]:
            raise ValueError("destination must be a file directly inside usb-games")
        self._reset_to_menu(reset)
        self._command(CMD_MKDIR, _encode_path("usb-games"))
        self._status("create usb-games directory", allowed=(0, 8))  # FR_EXIST
        self._command(CMD_FILE_OPEN, b"\x0A" + path)  # WRITE | CREATE_ALWAYS
        self._status("open ROM file")
        self._command(CMD_FILE_WRITE, struct.pack(">I", len(data)))
        for offset in range(0, len(data), BLOCK_SIZE):
            ack = self._read(1, "file write acknowledgement")
            if ack != b"\x00":
                raise ProtocolError(f"file write rejected at byte {offset}: status 0x{ack[0]:02X}")
            self._write(data[offset:offset + BLOCK_SIZE], "ROM transfer")
        self._status("write ROM file")
        self._command(CMD_FILE_CLOSE)
        self._status("close ROM file")
        self._fifo(b"*i")
        # Menu strings use separate FIFO writes for length and ASCII bytes.
        self._fifo(path[:2])
        self._fifo(path[2:])
        result = self._read(1, "ROM installation", timeout=30.0)
        if result != b"\x00":
            raise ProtocolError(f"ROM installation failed: status 0x{result[0]:02X}")
        self._fifo(b"*s")
        return {"destination": destination, "bytes": len(data), "reset": reset,
                "sha256": hashlib.sha256(data).hexdigest(), "installed": True,
                "launch_sent": True, "runtime_verified": False}


def _encode_path(path):
    if not path or any(ord(c) < 32 or ord(c) > 126 or c in "\\:*?\"<>|" for c in path):
        raise ValueError("cartridge path must use printable ASCII without FAT reserved characters")
    if any(part in ("", ".", "..") for part in path.split("/")):
        raise ValueError("cartridge path must not contain empty, dot, or parent components")
    encoded = path.encode("ascii")
    if len(encoded) > 255:
        raise ValueError("cartridge path is too long (maximum 255 ASCII bytes)")
    return struct.pack(">H", len(encoded)) + encoded


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


def deploy_rom(rom, port, reset="hard", timeout=120.0):
    rom = Path(rom).resolve()
    destination = "usb-games/" + rom.name
    _encode_path(destination)  # Reject unsafe names before opening the device.
    size = rom.stat().st_size
    if not rom.is_file() or not 0 < size <= 0xFFFFFFFF:
        raise ValueError("ROM must be a regular, nonempty file with a 32-bit size")
    data = rom.read_bytes()  # Host read errors must occur before reset or SD writes.
    with open_serial(port) as connection:
        client = MegaEverDrivePro(connection, timeout)
        identity = client.identify()
        return dict(identity, **client.deploy(data, destination, reset))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="Explicit native serial port; no auto-probing")
    parser.add_argument("--timeout", type=positive_seconds, default=120.0,
                        help="Total transaction deadline in seconds (default: 120)")
    commands = parser.add_subparsers(dest="operation", required=True)
    commands.add_parser("info", help="Verify Mega EverDrive Pro identity without reset/upload")
    verify = commands.add_parser("verify-lab", help="Verify the running Hardware Lab USB FIFO link")
    verify.add_argument("--duration", type=lab_duration, default=0.0,
                        help="Refresh verification for 0-300 seconds; 0 performs one handshake")
    deploy = commands.add_parser("deploy", help="Upload, install and send ROM launch")
    deploy.add_argument("rom", type=Path)
    deploy.add_argument("--reset", choices=("hard", "soft"), default="hard",
                        help="Default: hard; soft only on explicit request")
    args = parser.parse_args(argv)
    try:
        if args.operation == "deploy":
            result = deploy_rom(args.rom, args.port, args.reset, args.timeout)
        else:
            with open_serial(args.port) as connection:
                client = MegaEverDrivePro(connection, args.timeout)
                result = client.identify()
                if args.operation == "verify-lab":
                    result = dict(result, **client.verify_lab(args.duration))
        print(json.dumps(result, sort_keys=True))
        return 0
    except (OSError, ProtocolError, ValueError) as error:
        print(f"EverDrive: {error}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("EverDrive: interrupted; deployment did not complete", file=sys.stderr)
        return 130


if __name__ == "__main__":
    sys.exit(main())
