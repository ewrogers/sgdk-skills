# Working on SGDK Skills

- Keep skills portable and independently installable. Runtime references,
  helpers and templates belong inside their skill folder. Repository-level
  tests and tooling must not become installed-skill dependencies.
- Keep `SKILL.md` focused on decisions and workflow. Put detailed, sourced
  explanations in references; do not copy whole SDK manuals or teach generic C.
- Verify API names, signatures, resource syntax and behavior against the pinned
  Docker image's SGDK source/manuals. Cite versioned primary sources. If a
  feature is unverified, investigate it rather than inventing a plausible API.
- Separate Mega Drive hardware limits, SGDK behavior and optional game design
  conventions. Budget CPU, main RAM, VRAM, DMA and scanline rendering separately.
  Do not inherit AGES engine restrictions or impose a new game framework.
- Prefer optimized release builds for performance comparisons. Explain what a
  counter measures and distinguish build checks from emulator/hardware testing.
- Use portable Python with the standard library for small helpers when useful.
  Pass commands as argument lists, preserve exit codes and handle paths with
  spaces. Document OS-specific differences instead of claiming untested support.
- Do not overwrite existing projects or source art, edit host emulator settings,
  require privileged Docker, or install skills globally unless requested.
- Test observable behavior and useful failures. Compile executable examples and
  resource fixtures through the pinned container. Avoid tests of prose wording.
- Keep the README concise and synchronize examples, references and tests when
  behavior changes. Keep logs, ROMs, generated files and downloads out of Git.
- Changes are reviewed in PRs. Never claim runtime visuals, audio, timing or
  hardware behavior were tested when validation only compiled a ROM.
