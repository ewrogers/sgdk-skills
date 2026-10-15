"""Declared-budget arithmetic and validation; no performance claims or Docker needed."""

from __future__ import annotations

import copy
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "skills/sgdk-planning/scripts/budget_check.py"
SPEC = importlib.util.spec_from_file_location("budget_check", SCRIPT)
BUDGET = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = BUDGET
SPEC.loader.exec_module(BUDGET)


def declared_budget(mode: str = "H40") -> dict:
    """All fields are explicitly covered, unlike a typical incomplete first plan."""
    return {
        "schema_version": 1,
        "video_mode": mode,
        "ram": {"items": [{"name": "All main RAM", "bytes": 65536}], "complete": True},
        "vram": {"items": [{"name": "All VRAM", "tiles": 2048}], "complete": True},
        "palette_banks": 4,
        "sprites": {
            "total": 80 if mode == "H40" else 64,
            "per_scanline": 20 if mode == "H40" else 16,
            "pixels_per_scanline": 320 if mode == "H40" else 256,
        },
        "rom": {
            "bytes_before_padding": 4 * 1024 * 1024,
            "mapping": "standard",
            "cartridge_bytes": 4 * 1024 * 1024,
        },
        "dma": {"peak_bytes": 1024, "budget_bytes": 1024},
    }


def checks(document: dict) -> dict:
    return {check["name"]: check for check in BUDGET.assess(document)["checks"]}


class BudgetArithmeticTests(unittest.TestCase):
    def test_example_worksheet_keeps_unresolved_costs_visible(self):
        example = SCRIPT.parents[1] / "assets" / "budget-example.json"
        report = BUDGET.assess(json.loads(example.read_text(encoding="utf-8")))
        self.assertEqual(report["status"], "incomplete")
        metrics = {item["name"]: item for item in report["checks"]}
        self.assertEqual(metrics["ram"]["known"], 24 * 32)
        self.assertEqual(metrics["vram"]["known"], (420 + 160) * 32)
        for name in ["ram", "vram", "rom", "dma"]:
            with self.subTest(check=name):
                self.assertFalse(metrics[name]["complete"])
                self.assertIsNone(metrics[name]["remaining"])
        self.assertIn("rom.bytes_before_padding", report["unknowns"])

    def test_exact_h40_boundaries_fit_declared_limits(self):
        report = BUDGET.assess(declared_budget())
        self.assertEqual(report["status"], "within_declared_limits")
        self.assertEqual(report["unknowns"], [])
        self.assertEqual(len(report["checks"]), 8)
        self.assertEqual(
            {check["name"] for check in report["checks"]},
            {
                "ram", "vram", "palette_banks", "sprites_total",
                "sprites_per_scanline", "sprite_pixels_per_scanline", "rom", "dma",
            },
        )
        for check in report["checks"]:
            with self.subTest(check=check["name"]):
                self.assertTrue(check["complete"])
                self.assertFalse(check["over_limit"])
                self.assertEqual(check["remaining"], 0)

    def test_exact_h32_boundaries_are_distinct_from_h40(self):
        document = declared_budget("H32")
        self.assertEqual(BUDGET.assess(document)["status"], "within_declared_limits")
        metrics = checks(document)
        self.assertEqual(metrics["sprites_total"]["limit"], 64)
        self.assertEqual(metrics["sprites_per_scanline"]["limit"], 16)
        self.assertEqual(metrics["sprite_pixels_per_scanline"]["limit"], 256)

    def test_one_past_each_independent_limit_is_over(self):
        mutations = {
            "ram": lambda d: d["ram"]["items"][0].update(bytes=65537),
            "vram": lambda d: d["vram"]["items"][0].update(tiles=2049),
            "palette_banks": lambda d: d.update(palette_banks=5),
            "sprites_total": lambda d: d["sprites"].update(total=81),
            "sprites_per_scanline": lambda d: d["sprites"].update(per_scanline=21),
            "sprite_pixels_per_scanline": lambda d: d["sprites"].update(pixels_per_scanline=321),
            "rom": lambda d: d["rom"].update(bytes_before_padding=4194305),
            "dma": lambda d: d["dma"].update(peak_bytes=1025),
        }
        for name, mutate in mutations.items():
            with self.subTest(check=name):
                document = declared_budget()
                mutate(document)
                report = BUDGET.assess(document)
                self.assertEqual(report["status"], "over_limit")
                self.assertTrue(checks(document)[name]["over_limit"])

    def test_scanline_pixel_limit_can_fail_with_few_sprites(self):
        document = declared_budget("H32")
        document["sprites"] = {"total": 9, "per_scanline": 9, "pixels_per_scanline": 288}
        metrics = checks(document)
        self.assertFalse(metrics["sprites_total"]["over_limit"])
        self.assertFalse(metrics["sprites_per_scanline"]["over_limit"])
        self.assertTrue(metrics["sprite_pixels_per_scanline"]["over_limit"])

    def test_sprite_count_can_fail_while_scanline_pixels_fit(self):
        document = declared_budget()
        document["sprites"] = {"total": 21, "per_scanline": 21, "pixels_per_scanline": 168}
        metrics = checks(document)
        self.assertFalse(metrics["sprites_total"]["over_limit"])
        self.assertTrue(metrics["sprites_per_scanline"]["over_limit"])
        self.assertFalse(metrics["sprite_pixels_per_scanline"]["over_limit"])

    def test_memory_counts_tiles_and_bytes_are_added_once(self):
        document = declared_budget()
        document["vram"] = {
            "items": [
                {"name": "Patterns", "tiles": 100, "count": 2},
                {"name": "Plane tables", "bytes": 4096},
                {"name": "Other tables", "bytes": 512, "count": 3},
            ],
            "complete": True,
        }
        metric = checks(document)["vram"]
        self.assertEqual(metric["known"], 12032)
        self.assertEqual(metric["remaining"], 53504)

    def test_zero_values_and_empty_complete_sections_are_valid(self):
        document = declared_budget()
        document["ram"] = {"items": [], "complete": True}
        document["vram"] = {"items": [{"name": "No patterns", "tiles": 0}], "complete": True}
        document["palette_banks"] = 0
        document["sprites"] = {"total": 0, "per_scanline": 0, "pixels_per_scanline": 0}
        document["rom"]["bytes_before_padding"] = 0
        document["dma"]["peak_bytes"] = 0
        report = BUDGET.assess(document)
        self.assertEqual(report["status"], "within_declared_limits")
        self.assertEqual(checks(document)["ram"]["remaining"], 65536)

    def test_missing_sections_are_incomplete_without_free_memory_claim(self):
        document = {"schema_version": 1, "video_mode": "H40"}
        report = BUDGET.assess(document)
        self.assertEqual(report["status"], "incomplete")
        self.assertTrue(report["unknowns"])
        for metric in report["checks"]:
            self.assertFalse(metric["complete"])
            self.assertIsNone(metric["remaining"])
        self.assertEqual(checks(document)["ram"]["known"], 0)
        self.assertIsNone(checks(document)["dma"]["limit"])
        self.assertIsNone(checks(document)["dma"]["over_limit"])

    def test_memory_coverage_must_be_explicit(self):
        for coverage in (None, False):
            with self.subTest(complete=coverage):
                document = declared_budget()
                document["ram"] = {"items": [{"name": "Only my objects", "bytes": 128}]}
                if coverage is not None:
                    document["ram"]["complete"] = coverage
                report = BUDGET.assess(document)
                self.assertEqual(report["status"], "incomplete")
                metric = checks(document)["ram"]
                self.assertEqual(metric["known"], 128)
                self.assertFalse(metric["complete"])
                self.assertIsNone(metric["remaining"])

    def test_unknown_item_prevents_complete_accounting(self):
        document = declared_budget()
        document["ram"]["items"] = [
            {"name": "Objects", "bytes": 2048, "count": 2},
            {"name": "Stack and SGDK state", "bytes": None},
        ]
        report = BUDGET.assess(document)
        metric = checks(document)["ram"]
        self.assertEqual(metric["known"], 4096)
        self.assertFalse(metric["complete"])
        self.assertIsNone(metric["remaining"])
        self.assertEqual(report["status"], "incomplete")
        self.assertTrue(report["unknowns"])

    def test_known_partial_subtotal_can_already_exceed_limit(self):
        document = declared_budget()
        document["ram"] = {
            "items": [
                {"name": "Objects", "bytes": 70000},
                {"name": "Unknown stack", "bytes": None},
            ],
            "complete": False,
        }
        report = BUDGET.assess(document)
        self.assertEqual(report["status"], "over_limit")
        metric = checks(document)["ram"]
        self.assertEqual(metric["known"], 70000)
        self.assertTrue(metric["over_limit"])
        self.assertIsNone(metric["remaining"])

    def test_null_sprite_and_palette_metrics_are_unknown(self):
        document = declared_budget()
        document["palette_banks"] = None
        document["sprites"] = {"total": None, "per_scanline": None, "pixels_per_scanline": None}
        report = BUDGET.assess(document)
        self.assertEqual(report["status"], "incomplete")
        for name in ("palette_banks", "sprites_total", "sprites_per_scanline", "sprite_pixels_per_scanline"):
            metric = checks(document)[name]
            self.assertEqual(metric["known"], 0)
            self.assertFalse(metric["complete"])
            self.assertIsNone(metric["remaining"])

    def test_rom_padding_uses_ceiling_including_exact_alignment(self):
        for unpadded, padded in ((0, 0), (1, 131072), (131072, 131072), (131073, 262144)):
            with self.subTest(bytes_before_padding=unpadded):
                document = declared_budget()
                document["rom"]["bytes_before_padding"] = unpadded
                self.assertEqual(checks(document)["rom"]["known"], padded)

    def test_padding_can_exceed_cartridge_even_when_unpadded_data_fits(self):
        document = declared_budget()
        document["rom"] = {
            "bytes_before_padding": 200000,
            "cartridge_bytes": 200000,
            "mapping": "standard",
        }
        metric = checks(document)["rom"]
        self.assertEqual(metric["known"], 262144)
        self.assertEqual(metric["limit"], 200000)
        self.assertTrue(metric["over_limit"])

    def test_custom_rom_alignment_is_used_without_power_of_two_assumption(self):
        document = declared_budget()
        document["rom"] = {"bytes_before_padding": 301, "alignment_bytes": 100, "mapping": "standard"}
        self.assertEqual(checks(document)["rom"]["known"], 400)

    def test_standard_mapping_caps_rom_at_four_mib_even_for_larger_cart(self):
        document = declared_budget()
        document["rom"].update(bytes_before_padding=5 * 1024 * 1024, cartridge_bytes=8 * 1024 * 1024)
        metric = checks(document)["rom"]
        self.assertEqual(metric["limit"], 4 * 1024 * 1024)
        self.assertTrue(metric["over_limit"])

    def test_banked_mapping_uses_declared_cart_without_verifying_mapper(self):
        document = declared_budget()
        document["rom"].update(
            bytes_before_padding=5 * 1024 * 1024,
            cartridge_bytes=8 * 1024 * 1024,
            mapping="banked",
        )
        report = BUDGET.assess(document)
        metric = checks(document)["rom"]
        self.assertEqual(metric["limit"], 8 * 1024 * 1024)
        self.assertFalse(metric["over_limit"])
        self.assertTrue(report["notes"])

    def test_banked_mapping_without_cart_capacity_remains_incomplete(self):
        document = declared_budget()
        document["rom"] = {"bytes_before_padding": 5 * 1024 * 1024, "mapping": "banked"}
        metric = checks(document)["rom"]
        self.assertFalse(metric["complete"])
        self.assertIsNone(metric["limit"])
        self.assertIsNone(metric["remaining"])
        self.assertIsNone(metric["over_limit"])
        self.assertEqual(BUDGET.assess(document)["status"], "incomplete")

    def test_unknown_mapping_never_claims_complete_rom_coverage(self):
        for mapping in (None, "unknown"):
            with self.subTest(mapping=mapping):
                document = declared_budget()
                document["rom"].pop("mapping")
                if mapping is not None:
                    document["rom"]["mapping"] = mapping
                metric = checks(document)["rom"]
                self.assertFalse(metric["complete"])
                self.assertIsNone(metric["remaining"])
                self.assertEqual(BUDGET.assess(document)["status"], "incomplete")

    def test_dma_requires_explicit_transfer_budget(self):
        for dma in ({}, {"peak_bytes": 1000}, {"peak_bytes": None, "budget_bytes": 2000}):
            with self.subTest(dma=dma):
                document = declared_budget()
                document["dma"] = dma
                self.assertFalse(checks(document)["dma"]["complete"])
                self.assertIsNone(checks(document)["dma"]["remaining"])
                self.assertEqual(BUDGET.assess(document)["status"], "incomplete")

    def test_large_python_integers_do_not_overflow_arithmetic(self):
        document = declared_budget()
        document["ram"]["items"] = [{"name": "Impossible allocation", "bytes": 10**100, "count": 10**100}]
        document["rom"]["bytes_before_padding"] = 10**100
        report = BUDGET.assess(document)
        self.assertEqual(report["status"], "over_limit")
        self.assertEqual(checks(document)["ram"]["known"], 10**200)
        self.assertEqual(checks(document)["rom"]["known"] % 131072, 0)

    def test_assessment_does_not_modify_input(self):
        document = declared_budget()
        original = copy.deepcopy(document)
        BUDGET.assess(document)
        self.assertEqual(document, original)


class BudgetValidationTests(unittest.TestCase):
    def assert_invalid(self, document):
        with self.assertRaises(BUDGET.BudgetError):
            BUDGET.assess(document)

    def test_document_version_and_video_mode_are_required(self):
        for document in (None, [], {}, {"schema_version": 1}, {"video_mode": "H40"}):
            with self.subTest(document=document):
                self.assert_invalid(document)

    def test_rejects_unsupported_versions_and_video_modes(self):
        for key, values in {
            "schema_version": (True, 1.0, "1", 0, 2),
            "video_mode": (None, "h40", "H80", 40),
        }.items():
            for value in values:
                with self.subTest(key=key, value=value):
                    document = declared_budget()
                    document[key] = value
                    self.assert_invalid(document)

    def test_rejects_unknown_fields_at_each_level(self):
        locations = (
            lambda d: d,
            lambda d: d["ram"],
            lambda d: d["vram"],
            lambda d: d["ram"]["items"][0],
            lambda d: d["sprites"],
            lambda d: d["rom"],
            lambda d: d["dma"],
        )
        for index, location in enumerate(locations):
            with self.subTest(level=index):
                document = declared_budget()
                location(document)["typo"] = 1
                self.assert_invalid(document)

    def test_rejects_malformed_sections_and_item_shapes(self):
        for section in ("ram", "vram", "sprites", "rom", "dma"):
            for value in (None, [], "unknown", 0, True):
                with self.subTest(section=section, value=value):
                    document = declared_budget()
                    document[section] = value
                    self.assert_invalid(document)
        for items in (None, {}, "objects", [None], [1], ["objects"]):
            with self.subTest(items=items):
                document = declared_budget()
                document["ram"]["items"] = items
                self.assert_invalid(document)

    def test_memory_items_require_one_quantity(self):
        for section, item in (
            ("ram", {"name": "Objects"}),
            ("ram", {"name": "Objects", "tiles": 2}),
            ("vram", {"name": "Tiles"}),
            ("vram", {"name": "Tiles", "tiles": 2, "bytes": 64}),
        ):
            with self.subTest(section=section, item=item):
                document = declared_budget()
                document[section]["items"] = [item]
                self.assert_invalid(document)

    def test_memory_names_are_nonempty_unique_and_notes_are_text(self):
        for items in (
            [{"bytes": 1}],
            [{"name": "", "bytes": 1}],
            [{"name": "   ", "bytes": 1}],
            [{"name": 2, "bytes": 1}],
            [{"name": "Objects", "bytes": 1}, {"name": "objects", "bytes": 2}],
            [{"name": "Objects", "bytes": 1, "note": 123}],
        ):
            with self.subTest(items=items):
                document = declared_budget()
                document["ram"]["items"] = items
                self.assert_invalid(document)

    def test_nonnegative_counts_reject_bool_float_and_negative(self):
        setters = {
            "memory bytes": lambda d, v: d["ram"]["items"][0].update(bytes=v),
            "VRAM tiles": lambda d, v: d["vram"]["items"][0].update(tiles=v),
            "palette count": lambda d, v: d.update(palette_banks=v),
            "sprite total": lambda d, v: d["sprites"].update(total=v),
            "scanline count": lambda d, v: d["sprites"].update(per_scanline=v),
            "scanline pixels": lambda d, v: d["sprites"].update(pixels_per_scanline=v),
            "ROM bytes": lambda d, v: d["rom"].update(bytes_before_padding=v),
            "DMA peak": lambda d, v: d["dma"].update(peak_bytes=v),
        }
        for name, setter in setters.items():
            for value in (True, False, 1.0, -1, "1"):
                with self.subTest(field=name, value=value):
                    document = declared_budget()
                    setter(document, value)
                    self.assert_invalid(document)

    def test_positive_sizes_reject_zero_null_bool_float_and_negative(self):
        setters = {
            "item count": lambda d, v: d["ram"]["items"][0].update(count=v),
            "alignment": lambda d, v: d["rom"].update(alignment_bytes=v),
            "cartridge": lambda d, v: d["rom"].update(cartridge_bytes=v),
            "DMA budget": lambda d, v: d["dma"].update(budget_bytes=v),
        }
        for name, setter in setters.items():
            for value in (0, True, False, 1.0, -1, "1"):
                with self.subTest(field=name, value=value):
                    document = declared_budget()
                    setter(document, value)
                    self.assert_invalid(document)
        for setter in (setters["item count"], setters["alignment"]):
            document = declared_budget()
            setter(document, None)
            self.assert_invalid(document)

    def test_complete_requires_boolean_and_mapping_is_an_explicit_enum(self):
        for value in (None, 0, 1, "yes"):
            document = declared_budget()
            document["ram"]["complete"] = value
            self.assert_invalid(document)
        for value in (None, True, "automatic", "STANDARD"):
            document = declared_budget()
            document["rom"]["mapping"] = value
            self.assert_invalid(document)


class BudgetCommandTests(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory(prefix="sgdk planning test ")
        self.addCleanup(self.scratch.cleanup)
        self.folder = Path(self.scratch.name)
        self.input = self.folder / "my game budget.json"

    def run_cli(self, document=None, *, raw=None, script=SCRIPT, json_output=True):
        self.input.write_text(raw if raw is not None else json.dumps(document), encoding="utf-8")
        original = self.input.read_bytes()
        command = [sys.executable, str(script), str(self.input)]
        if json_output:
            command.append("--json")
        result = subprocess.run(command, cwd=self.folder, text=True, capture_output=True, check=False)
        self.assertEqual(self.input.read_bytes(), original)
        return result

    def test_cli_json_reports_declared_limits(self):
        result = self.run_cli(declared_budget())
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)["status"], "within_declared_limits")
        self.assertEqual(result.stderr, "")

    def test_incomplete_is_not_a_schema_error_or_an_over_limit_exit(self):
        result = self.run_cli({"schema_version": 1, "video_mode": "H40"})
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)["status"], "incomplete")

    def test_over_limit_has_distinct_nonzero_exit_and_report(self):
        document = declared_budget()
        document["palette_banks"] = 5
        result = self.run_cli(document)
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertEqual(json.loads(result.stdout)["status"], "over_limit")

    def test_malformed_json_and_schema_errors_exit_two_without_tracebacks(self):
        for raw in ("{", "[]", '{"schema_version":1,"video_mode":"H40","typo":1}'):
            with self.subTest(raw=raw):
                result = self.run_cli(raw=raw)
                self.assertEqual(result.returncode, 2)
                self.assertTrue(result.stderr)
                self.assertNotIn("Traceback", result.stderr)
                self.assertEqual(result.stdout, "")

    def test_duplicate_json_keys_at_top_and_nested_levels_are_rejected(self):
        for raw in (
            '{"schema_version":1,"video_mode":"H40","video_mode":"H32"}',
            '{"schema_version":1,"video_mode":"H40","ram":{"items":[],"complete":false,"complete":true}}',
        ):
            with self.subTest(raw=raw):
                result = self.run_cli(raw=raw)
                self.assertEqual(result.returncode, 2)
                self.assertTrue(result.stderr)
                self.assertEqual(result.stdout, "")

    def test_missing_file_is_actionable_without_python_traceback(self):
        result = subprocess.run(
            [sys.executable, str(SCRIPT), str(self.input)],
            text=True, capture_output=True, check=False,
        )
        self.assertEqual(result.returncode, 2)
        self.assertIn(self.input.name, result.stderr)
        self.assertNotIn("Traceback", result.stderr)

    def test_copied_helper_runs_independently_outside_repository(self):
        folder = self.folder / "independent installed skill" / "scripts"
        folder.mkdir(parents=True)
        copied = folder / SCRIPT.name
        shutil.copyfile(SCRIPT, copied)
        result = self.run_cli(declared_budget(), script=copied)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)["status"], "within_declared_limits")

    def test_default_human_output_is_nonempty(self):
        result = self.run_cli(declared_budget(), json_output=False)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(result.stdout.strip())
        self.assertEqual(result.stderr, "")


if __name__ == "__main__":
    unittest.main()
