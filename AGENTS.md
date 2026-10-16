# Working on SGDK Skills

- Keep these skills focused on SGDK cartridge development. The README may link
  to the separate Sega CD repositories; their setup and API docs belong there.
- Keep skills portable and independently installable. Runtime references,
  helpers and templates belong inside their skill folder. Repository-level
  tests and tooling must not become installed-skill dependencies.
- Keep `SKILL.md` focused on decisions and workflow. Put detailed, sourced
  explanations in references; do not copy whole SDK manuals or teach generic C.
- Use `ewrogers/sgdk:latest` in SGDK commands, helpers and CI defaults. Pull it
  before validation; do not hard-code image or platform-child digests. Record
  the actual tool versions when investigating a failure or comparing builds.
- Verify API names, signatures, resource syntax and behavior against the selected
  Docker image's SGDK source/manuals. Cite versioned primary sources. If a
  feature is unverified, investigate it rather than inventing a plausible API.
- Separate Mega Drive hardware limits, SGDK behavior and optional game design
  conventions. Budget CPU, main RAM, VRAM, DMA and scanline rendering separately.
  Do not inherit AGES engine restrictions or impose a new game framework.
- Genre skills provide patterns and solutions to Mega Drive development problems:
  mechanics, code, structure and behavior. Lab art, audio, mood and tuning are
  illustrative, never requirements for another game. Follow the project’s
  creative direction rather than copying a lab’s style.
- Prefer optimized release builds for performance comparisons. Explain what a
  counter measures and distinguish build checks from emulator/hardware testing.
- Use portable Python with the standard library for small helpers when useful.
  Pass commands as argument lists, preserve exit codes and handle paths with
  spaces. Document OS-specific differences instead of claiming untested support.
- Do not overwrite existing projects or source art, edit host emulator settings,
  require privileged Docker, or install skills globally unless requested.
- Test observable behavior and useful failures. Compile executable examples and
  resource fixtures through the selected container. Avoid tests of prose wording.
- Keep the README concise and synchronize examples, references and tests when
  behavior changes. Keep logs, ROMs, generated files and downloads out of Git.
- Changes are reviewed in PRs. Never claim runtime visuals, audio, timing or
  hardware behavior were tested when validation only compiled a ROM.
