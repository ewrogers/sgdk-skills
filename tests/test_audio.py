"""Exercise sound commands and original assets, then build through actual SGDK."""
import importlib.util
import io
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import wave


ROOT = Path(__file__).resolve().parents[1]
SKILL = ROOT / "skills/sgdk-audio"
LAB = SKILL / "assets/sound-lab"
FIXTURE = ROOT / "tests/fixtures/audio"
IMAGE = os.environ.get("SGDK_IMAGE", "ewrogers/sgdk:latest")


class AudioBehaviorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = next((p for name in ["clang", "cc", "gcc"] if (p := shutil.which(name))), None)
        if compiler is None:
            if os.environ.get("SGDK_REQUIRE_HOST_CC") == "1":
                raise RuntimeError("CI requires a C17 compiler for audio behavior tests")
            raise unittest.SkipTest("Audio behavior tests need clang, cc or gcc")
        (ROOT / "build").mkdir(exist_ok=True)
        cls.folder = tempfile.TemporaryDirectory(prefix="sgdk audio host ", dir=ROOT / "build")
        cls.addClassCleanup(cls.folder.cleanup)
        project = Path(cls.folder.name) / "copied lab"
        shutil.copytree(LAB, project)
        cls.binary = Path(cls.folder.name) / ("audio-test.exe" if os.name == "nt" else "audio-test")
        result = subprocess.run([compiler, "-std=c17", "-Wall", "-Wextra", "-Werror",
                                 "-I", str(FIXTURE), "-I", str(project / "inc"),
                                 str(project / "src/audio.c"), str(FIXTURE / "audio_test.c"),
                                 "-o", str(cls.binary)], capture_output=True, text=True, timeout=60)
        if result.returncode:
            raise RuntimeError((result.stdout + result.stderr)[-6000:])

    def check_case(self, name):
        result = subprocess.run([str(self.binary), name], capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stdout.strip(), name)

    def test_load_before_setters(self): self.check_case("init")
    def test_play_pending_is_not_completion_and_held_does_not_restart(self): self.check_case("start_pending")
    def test_completion_once_after_observed_playback(self): self.check_case("completion")
    def test_pause_and_resume_wait_for_driver_without_false_completion(self): self.check_case("pause_resume")
    def test_idle_pause_and_fade_do_nothing(self): self.check_case("idle_pause")
    def test_stop_owns_effect_channel_and_is_not_completion(self): self.check_case("stop")
    def test_loop_toggle_edges(self): self.check_case("loop")
    def test_volume_toggle_affects_music_only(self): self.check_case("volume")
    def test_effect_edges_channel_priority_and_array_lengths(self): self.check_case("effects")
    def test_rejection_does_not_retry_or_count_as_accepted(self): self.check_case("rejection")
    def test_simultaneous_effects_choose_alarm(self): self.check_case("effect_conflict")
    def test_fade_owns_transport_but_allows_effects(self): self.check_case("fade")
    def test_stop_wins_simultaneous_transport_requests(self): self.check_case("transport_conflict")
    def test_one_second_fades_use_detected_pal_ntsc_refresh(self): self.check_case("regions")
    def test_contention_and_dma_chords_consume_their_inputs(self): self.check_case("test_switches")
    def test_four_second_measurement_in_both_regions_and_counter_wrap(self): self.check_case("timing")


class AudioAssetTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        spec = importlib.util.spec_from_file_location("lab_audio", SKILL / "scripts/make_lab_audio.py")
        cls.generator = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cls.generator)

    def test_original_assets_reproduce_and_wav_samples_are_bounded(self):
        for name, data in self.generator.assets().items():
            with self.subTest(name=name):
                self.assertEqual(data, (LAB / "res" / name).read_bytes())
                if name.endswith(".wav"):
                    self.assertEqual(struct.unpack_from("<I", data, 4)[0] + 8, len(data))
                    self.assertEqual(len(data) % 2, 0)
                    with wave.open(io.BytesIO(data)) as wav:
                        self.assertEqual((wav.getnchannels(), wav.getsampwidth(), wav.getframerate()), (1, 1, 13300))
                        frames = wav.readframes(wav.getnframes())
                        self.assertEqual(len(frames), 3325 if name == "click.wav" else 13300)
                        self.assertGreater(min(frames), 0)
                        self.assertLess(max(frames), 255)
                        self.assertLessEqual(abs(frames[-1] - 128), 1)

    def test_vgm_timing_loop_fm_psg_and_dac_streams(self):
        data = self.generator.music()
        read = lambda offset: struct.unpack_from("<I", data, offset)[0]
        self.assertEqual(data[:4], b"Vgm ")
        self.assertEqual(read(4) + 4, len(data))
        self.assertEqual(read(8), 0x160)
        start = read(0x34) + 0x34
        self.assertEqual(start, 256)
        loop = read(0x1C) + 0x1C
        samples, cursor = 0, start
        fm, psg, streams, banks, frequencies = [], [], [], [], []
        while data[cursor] != 0x66:
            command = data[cursor]
            if command == 0x50:
                psg.append(data[cursor + 1])
                cursor += 2
            elif command == 0x52:
                fm.append((cursor, data[cursor+1], data[cursor+2]))
                cursor += 3
            elif command == 0x62:
                samples += 735
                cursor += 1
            elif command == 0x67:
                self.assertEqual(data[cursor+1:cursor+3], b"\x66\x00")
                length = read(cursor+3)
                banks.append(data[cursor+7:cursor+7+length])
                cursor += 7 + length
            elif command == 0x90:
                self.assertEqual(data[cursor+1:cursor+5], b"\0\x02\0\x2A")
                cursor += 5
            elif command == 0x91:
                self.assertEqual(data[cursor+1:cursor+5], b"\0\0\x01\0")
                cursor += 5
            elif command == 0x92:
                frequencies.append(read(cursor+2)); cursor += 6
            elif command == 0x93:
                self.assertEqual(data[cursor+1], 0)
                self.assertEqual(data[cursor+6], 1)
                streams.append((samples, read(cursor+2), read(cursor+7)))
                cursor += 11
            elif command == 0x94:
                self.assertEqual(data[cursor+1], 0); cursor += 2
            else:
                self.fail(f"Unexpected command {command:#x}")
        self.assertEqual(cursor, len(data) - 1)
        self.assertEqual(samples, 176400)
        self.assertEqual(samples, read(0x18))
        self.assertEqual(samples, read(0x20))
        self.assertEqual(read(0x24), 60)
        self.assertEqual(frequencies, [13300])
        self.assertEqual(len(banks), 1)
        self.assertEqual(len(banks[0]), 2926)
        self.assertLess(min(banks[0]), 80)
        self.assertGreater(max(banks[0]), 180)
        self.assertGreater(loop, start + len(banks[0]))
        self.assertEqual(len(streams), 16)
        for beat, (time, offset, length) in enumerate(streams):
            self.assertEqual(time, beat * 15 * 735)
            self.assertEqual((offset, length), (0, 1330) if beat % 2 == 0 else (1330, 1596))
            self.assertLessEqual(offset + length, len(banks[0]))
            self.assertLessEqual(abs(banks[0][offset+length-1] - 128), 1)
        patch = {register: value for position, register, value in fm if position < loop}
        for channel in [0, 1]:
            self.assertEqual(patch[0xB0 + channel] & 7, 4)
            self.assertLess(patch[0x40 + channel], 127)  # Audible modulator.
            self.assertLess(patch[0x48 + channel], 127)  # Audible carrier.
            self.assertEqual(patch[0xB4 + channel], 0xC0)
        self.assertEqual(sum(register == 0x28 and value in [0xF0, 0xF1]
                             for _, register, value in fm), 16)
        self.assertGreater(len({value for _, reg, value in fm if reg == 0xA0}), 4)
        self.assertIn(0xE7, psg)  # Noise accompaniment as well as tone0.
        self.assertIn(0x9A, psg)

    def test_standalone_generator_preserves_existing_files(self):
        (ROOT / "build").mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="sgdk audio art ", dir=ROOT / "build") as folder:
            root = Path(folder)
            script = root / "make_lab_audio.py"
            shutil.copy2(SKILL / "scripts/make_lab_audio.py", script)
            output = root / "new audio"
            result = subprocess.run([sys.executable, str(script), str(output)], capture_output=True, timeout=20)
            self.assertEqual(result.returncode, 0, result.stderr)
            (output / "music.vgm").write_bytes(b"user music")
            before = {p.name: p.read_bytes() for p in output.iterdir()}
            result = subprocess.run([sys.executable, str(script), str(output)], capture_output=True, timeout=20)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(before, {p.name: p.read_bytes() for p in output.iterdir()})


class RegionHeaderTests(unittest.TestCase):
    def test_disposable_region_headers_and_custom_header_protection(self):
        (ROOT / "build").mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="sound regions ", dir=ROOT / "build") as folder:
            root = Path(folder)
            sdk = root / "fake SDK"
            source = sdk / "src/boot/rom_head.c"
            source.parent.mkdir(parents=True)
            template = '#include "genesis.h"\nconst ROMHeader rom_header = {\n    "JUE             "\n};\n'
            source.write_text(template)
            project = root / "disposable project"
            project.mkdir()
            helper = root / "region_header.py"
            shutil.copy2(LAB / "tools/region_header.py", helper)
            for region, code in [("ntsc", "U"), ("pal", "E"), ("auto", "JUE")]:
                result = subprocess.run([sys.executable, str(helper), region, "--sdk", str(sdk),
                                         "--project", str(project)], capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                generated = (project / "src/boot/rom_head.c").read_text()
                self.assertIn('"' + code.ljust(16) + '"', generated)
                self.assertEqual(source.read_text(), template)
            output = project / "src/boot/rom_head.c"
            output.write_text("user's custom ROM header")
            result = subprocess.run([sys.executable, str(helper), "pal", "--sdk", str(sdk),
                                     "--project", str(project)], capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(output.read_text(), "user's custom ROM header")
            source.write_text("unfamiliar SDK header")
            result = subprocess.run([sys.executable, str(helper), "pal", "--sdk", str(sdk),
                                     "--project", str(project)], capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(output.read_text(), "user's custom ROM header")


@unittest.skipUnless(os.environ.get("SGDK_SKILLS_INTEGRATION") == "1", "Docker integration is opt-in")
class AudioContainerTests(unittest.TestCase):
    def test_sdk_api_builds_and_linked_audio_alignment(self):
        (ROOT / "build").mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="sgdk audio container ", dir=ROOT / "build") as folder:
            project = Path(folder) / "sound lab"
            shutil.copytree(LAB, project)
            base = ["docker", "run", "--rm"]
            if os.environ.get("SGDK_PLATFORM"):
                base += ["--platform", os.environ["SGDK_PLATFORM"]]
            if hasattr(os, "getuid"):
                base += ["--user", f"{os.getuid()}:{os.getgid()}"]
            base += ["--mount", f"type=bind,source={project},target=/workspace", IMAGE]

            def run(*args):
                result = subprocess.run(base + list(args), capture_output=True, text=True, timeout=240)
                self.assertEqual(result.returncode, 0, (result.stdout + result.stderr)[-8000:])
                return result.stdout

            header = (FIXTURE / "genesis.h").read_text()
            constants = re.findall(r"^#define ((?:Z80_|BUTTON_)\w*) (0x[0-9A-Fa-f]+|[0-9]+)$", header, re.M)
            constants += [("SOUND_PCM_CH_AUTO", -1), ("SOUND_PCM_CH1", 0), ("SOUND_PCM_CH2", 1),
                          ("SOUND_PCM_CH3", 2), ("SOUND_PCM_CH2_MSK", 2)]
            checks = [f'_Static_assert({name} == {value}, "{name}");' for name, value in constants]
            checks += re.findall(r"^(?:void|bool)[^\n]*\);$", header, re.M)
            probe = "#include <genesis.h>\n" + "\n".join(checks) + "\n" + """
void (*load_music)(const u8 *) = XGM2_load;
void (*play_track)(u16) = XGM2_playTrack;
void (*far_music)(const u8 *, u32) = XGM2_play_FAR;
u8 (*pcm_status)(u16) = XGM2_isPlayingPCM;
u32 (*elapsed)(void) = XGM2_getElapsed;
u16 (*load)(bool) = XGM2_getCPULoad;
u16 (*dma_wait)(bool) = XGM2_getDMAWaitTime;
void (*fade_pause)(u16) = XGM2_fadeOutAndPause;
void (*fade_to)(u16, u16, u16) = XGM2_fadeTo;
void (*xgm_play)(const u8 *) = XGM_startPlay;
void (*pcm_play)(const u8 *, u32, SoundPcmSampleRate, SoundPanning, bool) = SND_PCM_startPlay;
void (*dpcm_play)(const u8 *, u32, SoundPCMChannel, bool) = SND_DPCM2_startPlay;
void (*pcm4_play)(const u8 *, u32, SoundPCMChannel, bool) = SND_PCM4_startPlay;
"""
            (project / "api_probe.c").write_text(probe, encoding="utf-8")
            run("m68k-elf-gcc", "-m68000", "-std=gnu17", "-fno-builtin",
                "-Werror=implicit-function-declaration", "-Werror=incompatible-pointer-types",
                "-I/opt/sgdk/inc", "-I/opt/sgdk/res", "-fsyntax-only", "api_probe.c")
            for mode in ["release", "debug"]:
                with self.subTest(mode=mode):
                    run("build", mode)
                    rom = (project / "out/rom.bin").read_bytes()
                    self.assertEqual(rom[0x100:0x104], b"SEGA")
                    self.assertGreater(len(rom), 512)
                    self.assertEqual(len(rom) % 131072, 0)
            args = ["m68k-elf-gdb", "--batch", "-ex", "file out/rom.out"]
            for name in ["music", "click", "alarm"]:
                args += ["-ex", f'printf "ADDRESS:%lu\\n", (unsigned long)&{name}']
            output = run(*args)
            # ResComp assembly symbols lack array debug types. Take sizes from
            # its generated C declarations, addresses from the actual link.
            sizes = dict(re.findall(r"extern const u8 (\w+)\[(\d+)\];",
                                    (project / "res/resources.h").read_text()))
            addresses = [int(x) for x in re.findall(r"ADDRESS:(\d+)", output)]
            assets = list(zip(addresses, [int(sizes[name]) for name in ["music", "click", "alarm"]]))
            self.assertEqual(len(assets), 3)
            for address, size in assets:
                self.assertEqual(address % 256, 0)
                self.assertEqual(size % 256, 0)
                self.assertGreater(size, 0)
                self.assertLess(address + size, len(rom) + 1)
                self.assertGreater(len(set(rom[address:address + size])), 1)
            self.assertEqual([size for _, size in assets[1:]], [3328, 13312])
            for (address, size), raw_size in zip(assets[1:], [3325, 13300]):
                self.assertEqual(rom[address + raw_size:address + size], bytes(size - raw_size))
            compiled = rom[assets[0][0]:sum(assets[0])]
            self.assertEqual(compiled[0], 0x10)  # Compiled XGM2 has no ASCII ident.
            self.assertEqual(compiled[1] & 3, 0)  # NTSC source, single track.
            sample_blocks, fm_blocks, psg_blocks = struct.unpack_from("<HHH", compiled, 2)
            self.assertGreater(sample_blocks, 0)
            self.assertGreater(fm_blocks, 0)
            self.assertGreater(psg_blocks, 0)
            sample_offsets = [v for v in struct.unpack_from("<124H", compiled, 8) if v != 0xFFFF]
            self.assertEqual(len(sample_offsets), 3)  # Two drums plus final size marker.
            self.assertEqual(sample_offsets[0], 0)
            self.assertEqual(sample_offsets[-1], sample_blocks)
            self.assertLess(sample_offsets[0], sample_offsets[1])
            self.assertLess(sample_offsets[1], sample_offsets[2])
            for region, code in [("ntsc", b"U"), ("pal", b"E")]:
                run("python3", "tools/region_header.py", region)
                run("build", "clean")
                run("build", "release")
                regional = (project / "out/rom.bin").read_bytes()
                self.assertEqual(regional[0x1F0:0x200], code.ljust(16, b" "))


if __name__ == "__main__":
    unittest.main()
