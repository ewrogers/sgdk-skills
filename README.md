# SGDK Skills

Practical agent skills for making Sega Mega Drive and Genesis games with
[SGDK](https://github.com/Stephane-D/SGDK). Build through the
[SGDK Docker toolchain](https://github.com/ewrogers/sgdk-docker), keep your project
on your computer, and use an agent to help with setup, assets and debugging.

| Skill | What to ask it for |
| --- | --- |
| [sgdk-project](skills/sgdk-project/SKILL.md) | Start a project, build a ROM, use the toolchain or check the actual SDK API. |
| [sgdk-assets](skills/sgdk-assets/SKILL.md) | Import art and audio, prepare palettes and spritesheets, or fix a ResComp error. |
| [sgdk-debug](skills/sgdk-debug/SKILL.md) | Investigate a failed build, a ROM fault, a graphics problem or a slow frame. |

## Install

Use [Skills CLI](https://github.com/vercel-labs/skills) to choose the skills and
agents you want:

```sh
npx skills add ewrogers/sgdk-skills
```

To list the collection or install just one:

```sh
npx skills add ewrogers/sgdk-skills --list
npx skills add ewrogers/sgdk-skills --skill sgdk-project
```

The current installer needs Node.js 22.20 or later. The build workflow needs
Docker running with Linux containers. The optional helpers need Python 3.10+
and use its standard library. No native SGDK installation or API key is needed.

## Use

Ask your agent to use a skill for a concrete task. For example:

- "Use sgdk-project to create a Hello World ROM in a new folder."
- "Use sgdk-assets to import this spritesheet and check its palette budget."
- "Use sgdk-debug to investigate this fault using the matching ELF file."

Each skill works on its own. It includes focused references and, where useful,
a helper or small example. Agents check the selected container's headers,
manuals and samples before proposing unfamiliar API calls. The default is a
verified SGDK 2.11 image digest; an existing project's pin takes precedence.

The guidance accounts for CPU work, RAM, VRAM, palettes, sprite scanlines and
transfer bandwidth separately. It helps you use SGDK directly and leaves your
game's architecture to you.

## Tested support

Host helpers are tested on Windows, macOS and Linux. ROM/resource examples are
built on native Linux ARM64 and AMD64; Docker Desktop supplies the Linux
environment on other hosts. Windows Docker runtime testing is still separate
from the host helper checks.

Build tests check API compatibility, resources and diagnostics. They do not
prove correct visuals, sound or frame timing. Those need emulator or console
testing. See [testing and maintenance](docs/testing.md) for checks, image updates
and the exact validation boundaries.

The skills and original examples are MIT licensed. SGDK and the bundled
toolchain retain their own licenses.
