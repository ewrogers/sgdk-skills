# Prove the workflow with Menu Lab

Use an existing, already understood game when checking release tooling. This
repository's proof builds Menu Lab, the small title/options/HUD example, then
packages its actual ROM and matching release ELF. No new game code is needed.

Menu Lab belongs to the UI skill's examples, but this release skill does not
require that skill to be installed. Use a copy of Menu Lab if available, or
use an existing SGDK project you have permission to package. The helper itself
has no dependency on another skill or repository file.

## Make the candidate

Build a disposable project copy with `ewrogers/sgdk:latest` and preserve its
outputs. Write short release notes with its title-screen navigation, A/B/Start
controls, supported regions and the testing you actually performed. Copy the
project license and required SGDK notice; do not invent new rights for assets.

Inspect the ROM, then use the packaging command in
[Packaging and handoff](packaging.md). The stock sample ROM-end mismatch is
expected in this proof and should appear as a review warning. A real release
should review its project header before the final build.

## Check the result

Extract into a fresh directory. Compare the ROM's SHA-256 against both the
build output and `manifest.json`. Verify every included file's length/hash
and that the optional ELF is the preserved release ELF.

Load the extracted ROM in a host emulator. Try the title, options, score demo,
pause and results. These are the same game bytes you started with; packaging
must not change visuals or behavior. Header/checksum tests alone cannot make
that gameplay claim. Region and real-console coverage should be recorded
separately, without changing host emulator settings just to make a check pass.

Repository tests also compare the helper against SGDK's actual `sizebnd`
output on controlled inputs. They build Menu Lab in release and debug modes,
check both ROM files and preserve the release ROM/ELF in an archive before the
debug build can overwrite them. Native ARM64 and AMD64 CI run the same checks.
