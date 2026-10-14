"""Installed skill resources must not require files elsewhere in the repo."""
from pathlib import Path
import re
import unittest
from urllib.parse import unquote, urlsplit


SKILLS = Path(__file__).resolve().parents[1] / "skills"


class SkillResourcesTests(unittest.TestCase):
    def test_relative_markdown_links_resolve_inside_each_skill(self):
        for skill in SKILLS.iterdir():
            if not skill.is_dir():
                continue
            for document in skill.rglob("*.md"):
                for target in re.findall(r"!?\[[^\]]*\]\(([^)]+)\)", document.read_text(encoding="utf-8")):
                    parts = urlsplit(target)
                    if parts.scheme or parts.netloc or not parts.path:
                        continue
                    with self.subTest(document=document, target=target):
                        resource = (document.parent / unquote(parts.path)).resolve()
                        self.assertTrue(resource.is_relative_to(skill.resolve()), "Link depends on another installed skill or repository file")
                        self.assertTrue(resource.exists(), "Installed reference is missing")


if __name__ == "__main__":
    unittest.main()
