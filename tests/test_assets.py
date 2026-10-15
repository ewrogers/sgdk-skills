"""Compile real SGDK resources; opt in with SGDK_SKILLS_INTEGRATION=1."""

from __future__ import annotations

import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import unittest
import zlib


ROOT = Path(__file__).resolve().parents[1]
EXAMPLE = ROOT / "skills/sgdk-assets/assets/import-example"
DEFAULT_IMAGE = (
    "ewrogers/sgdk:latest"
)


def png_chunk(kind: bytes, data: bytes) -> bytes:
    return (
        struct.pack(">I", len(data))
        + kind
        + data
        + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)
    )


def indexed_png(width: int, height: int, palette_entries: int = 16) -> bytes:
    """Make an original indexed fixture with only indices 0 and 1 used."""
    palette = bytes([0, 0, 0, 68, 136, 204]) + bytes((palette_entries - 2) * 3)
    pixels = b"".join(b"\0" + bytes([1] * width) for _ in range(height))
    return (
        b"\x89PNG\r\n\x1a\n"
        + png_chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 3, 0, 0, 0))
        + png_chunk(b"PLTE", palette)
        + png_chunk(b"IDAT", zlib.compress(pixels))
        + png_chunk(b"IEND", b"")
    )


@unittest.skipUnless(
    os.environ.get("SGDK_SKILLS_INTEGRATION") == "1",
    "Set SGDK_SKILLS_INTEGRATION=1 to compile with Docker",
)
class AssetImportTests(unittest.TestCase):
    def setUp(self) -> None:
        self.scratch = tempfile.TemporaryDirectory(prefix="sgdk asset test ")
        self.addCleanup(self.scratch.cleanup)
        self.project = Path(self.scratch.name) / "project with spaces"
        shutil.copytree(EXAMPLE, self.project)
        self.image = os.environ.get("SGDK_IMAGE", DEFAULT_IMAGE)

    def docker(self, *arguments: str) -> subprocess.CompletedProcess[str]:
        command = ["docker", "run", "--rm"]
        if platform := os.environ.get("SGDK_PLATFORM"):
            command.extend(["--platform", platform])
        if hasattr(os, "getuid"):
            command.extend(["--user", f"{os.getuid()}:{os.getgid()}"])
        command.extend(
            ["--mount", f"type=bind,source={self.project},target=/workspace", self.image]
        )
        return subprocess.run(
            [*command, *arguments],
            capture_output=True,
            text=True,
            timeout=180,
            check=False,
        )

    def build(self) -> subprocess.CompletedProcess[str]:
        return self.docker("make", "-f", "/opt/sgdk/makefile.gen")

    def assert_build_succeeded(self) -> None:
        result = self.build()
        self.assertEqual(result.returncode, 0, (result.stdout + result.stderr)[-5000:])
        rom = (self.project / "out/rom.bin").read_bytes()
        self.assertGreater(len(rom), 512)
        self.assertEqual(rom[0x100:0x104], b"SEGA")

    def test_resource_example_compiles_and_links(self) -> None:
        self.assert_build_succeeded()
        header = (self.project / "res/resources.h").read_text()
        self.assertIn("SpriteDefinition marble", header)
        self.assertIn("Palette sprite_colors", header)
        self.assertIn("Image backdrop", header)

    def test_unused_palette_entries_do_not_reject_valid_pixels(self) -> None:
        # PLTE length alone must never become a made-up 16-color validator.
        (self.project / "res/gfx/marble.png").write_bytes(indexed_png(32, 16, 256))
        self.assert_build_succeeded()

    def test_non_tile_aligned_image_is_rejected(self) -> None:
        (self.project / "res/gfx/backdrop.png").write_bytes(indexed_png(9, 8))
        result = self.build()
        self.assertNotEqual(result.returncode, 0)
        self.assertRegex(result.stdout + result.stderr, r"(?i)(multiple|align|size|dimension)")
        self.assertFalse((self.project / "out/rom.bin").exists())

    def test_missing_resource_is_rejected(self) -> None:
        (self.project / "res/gfx/marble.png").unlink()
        result = self.build()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("marble.png", result.stdout + result.stderr)
        self.assertFalse((self.project / "out/rom.bin").exists())

    def test_malformed_declaration_is_rejected(self) -> None:
        (self.project / "res/resources.res").write_text('SPRITE marble "gfx/marble.png"\n')
        result = self.build()
        self.assertNotEqual(result.returncode, 0)
        self.assertRegex(result.stdout + result.stderr, r"(?i)(error|wrong|invalid|usage)")
        self.assertFalse((self.project / "out/rom.bin").exists())


if __name__ == "__main__":
    unittest.main()
