---
name: sgdk-release
description: Validate and package an existing SGDK game ROM for distribution, with ROM headers, SGDK checksums, cartridge size checks, release documents and matching-build records. Use when preparing a downloadable release or reviewing a release candidate, rather than implementing gameplay or publishing a Docker image.
---

# SGDK Release

Help someone ship the game they actually tested. Build a release candidate,
check the exact ROM, then package those bytes with clear controls and notices.
Do not silently fix headers, trim padding or rebuild after playtesting and
call the replacement the same candidate.

## Build and keep the evidence

Use `ewrogers/sgdk:latest`, pulling before the clean build. Follow the project's
Makefile if it has one. Record the source revision, uncommitted changes, image
identity, SDK/compiler versions and build command. A floating `latest` tag is
convenient for builds; its name alone cannot identify an old release.

Keep `out/rom.bin`, its matching `out/rom.out` and available symbol/map files
from that build before building another mode. Release symbols can help locate
a fault but may lack debug source lines. A separate debug build is useful for
investigation; it is not a matching symbol file for the release ROM.

- [ROM checks](references/rom-validation.md): SGDK's checksum, header defaults,
  reset vectors, padding, SRAM overlap and banked-cartridge boundaries.
- [Packaging and handoff](references/packaging.md): the standalone helper,
  repeatable archives, documents, provenance and testing before publication.
- [Menu Lab proof](references/menu-lab-proof.md): package a real existing Lab
  without creating another demo or making this skill depend on a sibling.

## Validate before packaging

The Python 3.10+ helper is [scripts/release.py](scripts/release.py). Locate it
relative to this installed skill. It reads files without patching them:

```sh
python3 /path/to/sgdk-release/scripts/release.py inspect out/rom.bin
```

Its profile is standard SGDK linear Mega Drive output, up to 4 MiB, with
128 KiB padding and the SDK's XOR-folded checksum. It is not a universal ROM
checker. Larger, banked, custom-padded or differently checksummed games need
their own documented validation; do not modify a game just to satisfy this helper.

Resolve errors at their source and rebuild. Review warnings: SGDK's stock
ROM-end field can differ from file size, and a declared SRAM range can overlap
ROM. A passing check proves neither playable regions nor working save storage.

## Hand off a playable release

Package the exact candidate with its version, controls, supported regions,
known limits, project license and required third-party notices. Preserve
existing project licensing. MIT applies to this skill and its original helpers,
not automatically to every game, asset or bundled dependency.

Test the packaged ROM after extraction. Record emulator/version, regions and
what was tried: startup, inputs, audio, menus, saves and a representative busy
scene. Distinguish these checks from real-console and physical-cart testing.
Do not promise hardware compatibility or a frame rate from a file checksum.

Create local artifacts for review. Preparing a release does not itself authorize
publishing it, uploading symbols, tagging the repository or changing CI credentials.
