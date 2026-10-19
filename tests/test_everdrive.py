"""Protocol transcripts and native serial integration; no console is contacted."""
from collections import deque
import contextlib
import importlib.util
import io
import json
import os
from pathlib import Path
import select
import shutil
import struct
import subprocess
import sys
import tempfile
import threading
import unittest
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[1]
SKILL = ROOT / "skills/sgdk-project"
SPEC = importlib.util.spec_from_file_location("everdrive", SKILL / "scripts/everdrive.py")
usb = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(usb)


def frame(command, payload=b""):
    return bytes((0x2B, 0xD4, command, command ^ 0xFF)) + payload


def memory(address, data):
    return frame(0x1A, struct.pack(">IIB", address, len(data), 0) + data)


def identity(protocol=5, device=0x18):
    return [(bytes(66), b""), (frame(0x40), bytes((0x5A, protocol, device, 0))),
            (frame(0x10), b"\xA5\x00")]


def legacy_identity(device=0x18):
    info = bytearray(64)
    info[44:50] = bytes.fromhex("011200010101")
    info[50] = device
    return [(bytes(66), b""), (frame(0x40), b""),
            (frame(0x10), b"\xA5\x00"), (frame(0x26), bytes(info))]


def deployment(data, reset=2, install_status=0):
    """Golden Gen2 transcript, based on the pinned official wire operations."""
    name = b"usb-games/Test ROM.bin"
    steps = identity() + [
        (frame(0x29, bytes((reset,))), b""),
        (memory(0x1800000, bytes(256)), b""),
        (frame(0x29, b"\x00"), b"r"),
        (frame(0xD2, b"\x00\x09usb-games"), b""),
        (frame(0x10), b"\xA5\x08"),  # Directory already exists is acceptable.
        (frame(0xC9, b"\x0A\x00\x16" + name), b""),
        (frame(0x10), b"\xA5\x00"),
        (frame(0xCC, struct.pack(">I", len(data))), b"\x00"),
    ]
    for offset in range(0, len(data), 1024):
        steps.append((data[offset:offset + 1024], b"\x00" if offset + 1024 < len(data) else b""))
    steps += [
        (frame(0x10), b"\xA5\x00"),
        (frame(0xCE), b""),
        (frame(0x10), b"\xA5\x00"),
        (memory(0x1810000, b"*i"), b""),
        (memory(0x1810000, b"\x00\x16"), b""),
        (memory(0x1810000, name), bytes((install_status,))),
    ]
    if install_status == 0:
        steps.append((memory(0x1810000, b"*s"), b""))
    return steps


class TranscriptPort:
    """A peer that releases responses only after the expected request arrives."""

    def __init__(self, steps, read_fragment=65536, write_fragment=65536):
        self.steps = deque(steps)
        self.read_fragment = read_fragment
        self.write_fragment = write_fragment
        self.expected_offset = 0
        self.received = bytearray()
        self.responses = bytearray()
        self.write_sizes = []

    def reset_input_buffer(self):
        self.responses.clear()

    def write(self, data):
        size = min(len(data), self.write_fragment)
        self.write_sizes.append(len(data))
        data = data[:size]
        self.received.extend(data)
        while data:
            if not self.steps:
                raise AssertionError(f"unexpected request {data.hex()}")
            expected, response = self.steps[0]
            length = min(len(data), len(expected) - self.expected_offset)
            actual = data[:length]
            want = expected[self.expected_offset:self.expected_offset + length]
            if actual != want:
                raise AssertionError(f"wire mismatch: got {actual.hex()}, expected {want.hex()}")
            self.expected_offset += length
            data = data[length:]
            if self.expected_offset == len(expected):
                self.responses.extend(response)
                self.steps.popleft()
                self.expected_offset = 0
        return size

    def read(self, size):
        length = min(size, self.read_fragment, len(self.responses))
        result = bytes(self.responses[:length])
        del self.responses[:length]
        return result


class EverDriveProtocolTests(unittest.TestCase):
    def test_hard_reset_upload_install_and_launch_with_fragmented_io(self):
        data = bytes(range(256)) * 10 + b"tail"  # Multiple ACKs and a short last block.
        port = TranscriptPort(deployment(data), read_fragment=1, write_fragment=7)
        client = usb.MegaEverDrivePro(port)
        with patch.object(usb.time, "sleep"):
            self.assertEqual(client.identify()["device_id"], 0x18)
            result = client.deploy(data, "usb-games/Test ROM.bin")
        self.assertFalse(port.steps)
        self.assertFalse(port.responses)
        self.assertTrue(result["installed"])
        self.assertTrue(result["launch_sent"])
        self.assertFalse(result["runtime_verified"])
        self.assertEqual(result["reset"], "hard")
        self.assertEqual(result["sha256"], usb.hashlib.sha256(data).hexdigest())

    def test_soft_requires_explicit_override_and_512_byte_usb_writes_are_split(self):
        data = b"x" * 512
        port = TranscriptPort(deployment(data, reset=1))
        client = usb.MegaEverDrivePro(port)
        with patch.object(usb.time, "sleep"):
            client.identify()
            self.assertEqual(client.deploy(data, "usb-games/Test ROM.bin", "soft")["reset"], "soft")
        self.assertFalse(port.steps)
        self.assertNotIn(512, port.write_sizes)

    def test_wrong_cartridge_or_protocol_is_rejected_before_reset(self):
        for protocol, device in [(5, 0x25), (6, 0x17)]:
            with self.subTest(protocol=protocol, device=device):
                port = TranscriptPort(identity(protocol, device))
                with self.assertRaisesRegex(usb.ProtocolError, "expected Mega EverDrive Pro"):
                    usb.MegaEverDrivePro(port).identify()
                self.assertNotIn(frame(0x29), port.received)

    def test_soft_override_does_not_change_the_next_deployment_default(self):
        port = TranscriptPort(deployment(b"rom", reset=1) + deployment(b"rom")[3:])
        client = usb.MegaEverDrivePro(port)
        with patch.object(usb.time, "sleep"):
            client.identify()
            client.deploy(b"rom", "usb-games/Test ROM.bin", reset="soft")
            self.assertEqual(client.deploy(b"rom", "usb-games/Test ROM.bin")["reset"], "hard")
        self.assertFalse(port.steps)

    def test_legacy_pro_identity_is_verified_via_system_info_before_deployment(self):
        port = TranscriptPort(legacy_identity() + deployment(b"rom")[3:], read_fragment=1)
        client = usb.MegaEverDrivePro(port)
        with patch.object(usb.time, "sleep"):
            identity = client.identify()
            self.assertEqual(identity["status_format"], "legacy")
            self.assertEqual(identity["mcu_version"], "0x0112")
            self.assertEqual(client.deploy(b"rom", "usb-games/Test ROM.bin")["reset"], "hard")
        self.assertFalse(port.steps)

    def test_legacy_non_pro_device_is_rejected_before_reset(self):
        port = TranscriptPort(legacy_identity(device=0x25))
        with self.assertRaisesRegex(usb.ProtocolError, "expected Mega EverDrive Pro"):
            usb.MegaEverDrivePro(port).identify()
        self.assertFalse(port.steps)
        self.assertNotIn(frame(0x29), port.received)

    def test_legacy_firmware_without_system_info_is_rejected(self):
        port = TranscriptPort([(bytes(66), b""), (frame(0x40), b""),
                               (frame(0x10), b"\xA5\x00"), (frame(0x26), b"")])
        with self.assertRaisesRegex(usb.ProtocolError, "legacy cartridge system information"):
            usb.MegaEverDrivePro(port).identify()
        self.assertFalse(port.steps)

    def test_commands_require_verified_identity(self):
        port = TranscriptPort([])
        with self.assertRaisesRegex(usb.ProtocolError, "identify"):
            usb.MegaEverDrivePro(port).deploy(b"rom", "usb-games/test.bin")
        self.assertFalse(port.received)

    def test_installation_failure_never_sends_launch(self):
        port = TranscriptPort(deployment(b"rom", install_status=9))
        client = usb.MegaEverDrivePro(port)
        with patch.object(usb.time, "sleep"):
            client.identify()
            with self.assertRaisesRegex(usb.ProtocolError, "installation failed.*09"):
                client.deploy(b"rom", "usb-games/Test ROM.bin")
        self.assertFalse(port.steps)
        self.assertNotIn(memory(0x1810000, b"*s"), port.received)

    def test_sd_failures_and_bad_ack_stop_without_installation(self):
        data = b"rom"
        for response_index, response, message in [
            (7, b"\xA5\x03", "directory"),
            (9, b"\xA5\x04", "open ROM"),
            (10, b"\x07", "write rejected"),
            (12, b"\xA5\x01", "write ROM"),
            (14, b"\xA5\x01", "close ROM"),
            (12, b"\x99\x00", "invalid.*status"),
        ]:
            with self.subTest(message=message):
                steps = deployment(data)
                request, _ = steps[response_index]
                steps[response_index] = (request, response)
                port = TranscriptPort(steps[:response_index + 1])
                client = usb.MegaEverDrivePro(port)
                with patch.object(usb.time, "sleep"):
                    client.identify()
                    with self.assertRaisesRegex(usb.ProtocolError, message):
                        client.deploy(data, "usb-games/Test ROM.bin")
                self.assertFalse(port.steps)
                self.assertNotIn(memory(0x1810000, b"*i"), port.received)

    def test_interruption_or_failure_while_reset_is_held_releases_it(self):
        for error in [OSError("disconnected"), KeyboardInterrupt()]:
            port = TranscriptPort(identity() + [(frame(0x29, b"\x02"), b""),
                                                (frame(0x29, b"\x00"), b"")])
            client = usb.MegaEverDrivePro(port)
            client.identify()
            with patch.object(usb.time, "sleep"), patch.object(client, "_memory_write", side_effect=error):
                with self.assertRaises(type(error)):
                    client.deploy(b"rom", "usb-games/test.bin")
            self.assertFalse(client.reset_asserted)
            self.assertFalse(port.steps)

    def test_expired_deadline_still_releases_reset(self):
        port = TranscriptPort(identity() + [(frame(0x29, b"\x02"), b""),
                                            (frame(0x29, b"\x00"), b"")])
        client = usb.MegaEverDrivePro(port)
        client.identify()

        def expire(seconds):
            client.deadline = 0

        with patch.object(usb.time, "sleep", side_effect=expire):
            with self.assertRaisesRegex(usb.ProtocolError, "timeout"):
                client.deploy(b"rom", "usb-games/test.bin")
        self.assertFalse(port.steps)
        self.assertFalse(client.reset_asserted)

    def test_missing_menu_response_does_not_fall_back_to_soft_reset(self):
        steps = deployment(b"rom")[:6]
        steps[-1] = (frame(0x29, b"\x00"), b"")
        port = TranscriptPort(steps)
        client = usb.MegaEverDrivePro(port)
        with patch.object(usb.time, "sleep"):
            client.identify()
            with self.assertRaisesRegex(usb.ProtocolError, "menu ready"):
                client.deploy(b"rom", "usb-games/Test ROM.bin")
        self.assertFalse(port.steps)
        self.assertNotIn(frame(0x29, b"\x01"), port.received)

    def test_invalid_rom_or_filename_never_opens_port(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for name, data in [("empty.bin", b""), ("café.bin", b"rom")]:
                rom = root / name
                rom.write_bytes(data)
                with patch.object(usb, "open_serial") as opening:
                    with self.assertRaises(ValueError):
                        usb.deploy_rom(rom, "unused")
                    opening.assert_not_called()
            with patch.object(usb, "open_serial") as opening:
                with self.assertRaises(OSError):
                    usb.deploy_rom(root / "missing.bin", "unused")
                opening.assert_not_called()

    def test_cli_reports_connection_errors_with_nonzero_exit(self):
        stderr = io.StringIO()
        with patch.object(usb, "open_serial", side_effect=OSError("port busy")):
            with contextlib.redirect_stderr(stderr):
                self.assertEqual(usb.main(["--port", "unused", "info"]), 1)
        self.assertIn("port busy", stderr.getvalue())

    def test_invalid_timeout_is_rejected_before_connection(self):
        for value in ["nan", "inf", "0", "-1"]:
            with patch.object(usb, "open_serial") as opening, contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit):
                    usb.main(["--port", "unused", "--timeout", value, "info"])
                opening.assert_not_called()

    def test_missing_serial_dependency_has_actionable_error(self):
        with patch.dict(sys.modules, {"serial": None}):
            with self.assertRaisesRegex(usb.ProtocolError, "requirements-usb.txt"):
                with usb.open_serial("unused"):
                    self.fail("must not open a port")


class HardwareLabLinkTests(unittest.TestCase):
    @staticmethod
    def packet(kind, nonce):
        import binascii
        body = b"HLV1" + struct.pack(">HI", kind, nonce)
        return body + struct.pack(">H", binascii.crc_hqx(body, 0xffff))

    def exchange(self, nonce, first_reply=None):
        reply = self.packet(2, nonce ^ 0xA55A5AA5) if first_reply is None else first_reply
        return [(memory(0x1810000, self.packet(1, nonce)), reply),
                (memory(0x1810000, self.packet(3, nonce)), self.packet(4, nonce))]

    def test_fragmented_exchange_waits_for_matching_response_and_guest_ack(self):
        response = b"noise" + self.packet(2, 0x777) + self.packet(2, 0x12345678 ^ 0xA55A5AA5)
        port = TranscriptPort(identity() + self.exchange(0x12345678,response), read_fragment=1, write_fragment=3)
        client = usb.MegaEverDrivePro(port);client.identify()
        with patch.object(usb.secrets, "randbelow", return_value=0x12345677):
            result = client.verify_lab()
        self.assertTrue(result["lab_link_verified"])
        self.assertEqual((result["handshakes"], result["lease_seconds"]), (1, 5))
        self.assertEqual(result["console_model"], "unknown")
        self.assertFalse(port.steps);self.assertFalse(port.responses)

    def test_stale_or_corrupt_response_does_not_send_ack(self):
        bad = bytearray(self.packet(2, 0x123 ^ 0xA55A5AA5));bad[-1] ^= 1
        for reply in [self.packet(4,0x123),bytes(bad),self.packet(2,0x124 ^ 0xA55A5AA5)]:
            port = TranscriptPort(identity()+self.exchange(0x123,reply)[:1])
            client = usb.MegaEverDrivePro(port);client.identify()
            with patch.object(usb.secrets, "randbelow", return_value=0x122):
                with self.assertRaisesRegex(usb.ProtocolError, "handshake"):client.verify_lab()
            self.assertFalse(port.steps)

    def test_missing_final_guest_ack_is_a_failure(self):
        steps = self.exchange(0x123);steps[-1] = (steps[-1][0],b"")
        port = TranscriptPort(identity()+steps)
        client = usb.MegaEverDrivePro(port);client.identify()
        with patch.object(usb.secrets, "randbelow", return_value=0x122):
            with self.assertRaises(usb.ProtocolError):client.verify_lab()
        self.assertFalse(port.steps)

    def test_absent_guest_times_out_without_ack_reset_or_sd_writes(self):
        port = TranscriptPort(identity()+self.exchange(0x123,b"")[:1])
        client = usb.MegaEverDrivePro(port);client.identify()
        with patch.object(usb.secrets, "randbelow", return_value=0x122):
            with self.assertRaises(usb.ProtocolError):client.verify_lab()
        self.assertFalse(port.steps)

    def test_duration_renews_with_a_fresh_nonce(self):
        port = TranscriptPort(identity()+self.exchange(0x123)+self.exchange(0x124))
        now = [10.0]
        with patch.object(usb.time, "monotonic", side_effect=lambda: now[0]), patch.object(usb.time, "sleep", side_effect=lambda seconds: now.__setitem__(0, now[0]+seconds)), patch.object(usb.secrets, "randbelow", return_value=0x122):
            client = usb.MegaEverDrivePro(port);client.identify()
            result = client.verify_lab(duration=1)
        self.assertEqual(result["handshakes"],2);self.assertFalse(port.steps)

    def test_unidentified_or_expired_transport_cannot_verify(self):
        port = TranscriptPort([]);client = usb.MegaEverDrivePro(port)
        with self.assertRaisesRegex(usb.ProtocolError,"identify"):client.verify_lab()
        client.identified=True;client.deadline=0
        with self.assertRaisesRegex(usb.ProtocolError,"timeout"):client.verify_lab()
        self.assertFalse(port.received)

    def test_invalid_duration_never_opens_the_port(self):
        for value in ["nan","inf","-1","301"]:
            with patch.object(usb,"open_serial") as opening, contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit):usb.main(["--port","unused","verify-lab","--duration",value])
                opening.assert_not_called()


@unittest.skipUnless(os.name == "posix" and importlib.util.find_spec("serial"),
                     "native pseudo-terminal integration needs POSIX and pySerial")
class NativeSerialTests(unittest.TestCase):
    def test_installed_cli_deploys_over_real_serial_api_to_simulated_cartridge(self):
        import pty
        master, slave = pty.openpty()
        port = os.ttyname(slave)
        data = bytes(range(256)) * 6  # Includes a 512-byte final transfer block.
        errors = []
        steps = deployment(data)

        def peer():
            try:
                for expected, response in steps:
                    received = bytearray()
                    while len(received) < len(expected):
                        if not select.select([master], [], [], 8)[0]:
                            raise AssertionError("host did not send the next protocol request")
                        received.extend(os.read(master, len(expected) - len(received)))
                    if received != expected:
                        raise AssertionError(f"request mismatch: {bytes(received).hex()}")
                    if response:
                        os.write(master, response)
            except BaseException as error:
                errors.append(error)

        thread = threading.Thread(target=peer, daemon=True)
        try:
            with tempfile.TemporaryDirectory(prefix="installed USB skill ") as temporary:
                installed = Path(temporary) / "sgdk-project"
                shutil.copytree(SKILL, installed, ignore=shutil.ignore_patterns("__pycache__"))
                rom = Path(temporary) / "Test ROM.bin"
                rom.write_bytes(data)
                thread.start()
                command = [sys.executable, str(installed / "scripts/everdrive.py")]
                if sys.platform == "darwin":
                    # macOS PTYs reject the custom baud ioctl used for 921600.
                    # Change only the PTY line speed; retain real pySerial I/O
                    # and run the copied CLI in its own process.
                    bootstrap = ("import runpy,sys; ns=runpy.run_path(sys.argv[1]); "
                                 "ns['main'].__globals__['BAUDRATE']=115200; "
                                 "raise SystemExit(ns['main'](sys.argv[2:]))")
                    command = [sys.executable, "-c", bootstrap, command[1]]
                result = subprocess.run(command + [
                                         "--port", port, "--timeout", "10", "deploy", str(rom)],
                                        cwd=temporary, capture_output=True, text=True, timeout=15)
                self.assertEqual(result.returncode, 0, result.stderr)
                status = json.loads(result.stdout)
                self.assertTrue(status["installed"])
                self.assertTrue(status["launch_sent"])
                self.assertFalse(status["runtime_verified"])
                self.assertEqual(status["reset"], "hard")
                thread.join(timeout=1)
                self.assertFalse(thread.is_alive(), "simulated cartridge did not finish")
                self.assertEqual(errors, [])
        finally:
            os.close(slave)
            os.close(master)


if __name__ == "__main__":
    unittest.main()
