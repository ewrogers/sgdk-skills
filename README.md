# SGDK Skills

[![skills.sh](https://skills.sh/b/ewrogers/sgdk-skills)](https://skills.sh/ewrogers/sgdk-skills)

Practical agent skills for making Sega Mega Drive and Genesis games with
[SGDK](https://github.com/Stephane-D/SGDK). Build through the
[SGDK Docker toolchain](https://github.com/ewrogers/sgdk-docker), keep your project
on your computer, and use an agent to help with setup, assets and debugging.

For Sega CD development, see [Megadev Docker](https://github.com/ewrogers/megadev-docker)
and [Megadev Skills](https://github.com/ewrogers/megadev-skills) (in development).

## Skills

### Get started

| Skill | What to ask it for |
| --- | --- |
| [sgdk-project](skills/sgdk-project/SKILL.md) | Start a project, build a ROM, use the toolchain or check the actual SDK API. |
| [sgdk-planning](skills/sgdk-planning/SKILL.md) | Turn a game idea into a first playable milestone, hardware budgets and practical tests. |
| [sgdk-best-practices](skills/sgdk-best-practices/SKILL.md) | Write or review C contracts, module boundaries and safe SGDK resource lifetimes. |

### Build your game

| Skill | What to ask it for |
| --- | --- |
| [sgdk-assets](skills/sgdk-assets/SKILL.md) | Import art and audio, prepare palettes and spritesheets, or fix a ResComp error. |
| [sgdk-pixellab](skills/sgdk-pixellab/SKILL.md) | Generate native art with cohesive palettes, prompt trials and SGDK validation; includes a playable JRPG art catalog. |
| [sgdk-graphics](skills/sgdk-graphics/SKILL.md) | Display sprites, play animations, scroll backgrounds and add palette effects. |
| [sgdk-animation](skills/sgdk-animation/SKILL.md) | Connect gameplay actions to animation, timed events and hitboxes. Includes Animation Lab. |
| [sgdk-effects](skills/sgdk-effects/SKILL.md) | Add particles, explosions, trails, weather and screen shake with bounded rendering. Includes Effects Lab. |
| [sgdk-raster](skills/sgdk-raster/SKILL.md) | Add per-line waves, waterline palette effects and split-screen cameras with explicit timing budgets. |
| [sgdk-input](skills/sgdk-input/SKILL.md) | Add pad, Mega Mouse or multitap controls, action bindings and reliable frame snapshots. |
| [sgdk-ui](skills/sgdk-ui/SKILL.md) | Add fonts, readable text, controller menus, pause screens and fixed score HUDs. |
| [sgdk-localization](skills/sgdk-localization/SKILL.md) | Add translated text, accents, bounded substitutions and font/layout budgets. Includes Language Lab. |
| [sgdk-dialogue](skills/sgdk-dialogue/SKILL.md) | Add text boxes, animated portraits, gradual styled text, choices and branching. |
| [sgdk-levels](skills/sgdk-levels/SKILL.md) | Import Tiled levels, stream large maps, and keep collision and cameras aligned. |
| [sgdk-streaming](skills/sgdk-streaming/SKILL.md) | Stream area art through tile caches, keep shared art resident and budget uploads. Includes Streaming Lab. |
| [sgdk-gameplay](skills/sgdk-gameplay/SKILL.md) | Add movement, collision, camera following and reusable object pools with consistent timing. |
| [sgdk-math](skills/sgdk-math/SKILL.md) | Use fixed-point numbers, angles, aiming, interpolation and perspective effects without guessing at precision or range. |
| [sgdk-ai](skills/sgdk-ai/SKILL.md) | Build enemy states, sight/detection, target selection and bounded navigation with predictable search work. |
| [sgdk-scenes](skills/sgdk-scenes/SKILL.md) | Build game flow, character selection, pause, Continue and safe resource handoffs between screens. |
| [sgdk-audio](skills/sgdk-audio/SKILL.md) | Choose a sound driver, play music and effects, and manage priorities and transitions. |
| [sgdk-save](skills/sgdk-save/SKILL.md) | Save progress to SRAM, recover damaged saves and add controller-friendly passwords. |
| [sgdk-cartridge](skills/sgdk-cartridge/SKILL.md) | Bank large ROMs, keep shared content and audio reachable, and save without disturbing mappings. |

### Game genres

| Skill | What to ask it for |
| --- | --- |
| [sgdk-platformer](skills/sgdk-platformer/SKILL.md) | Tune jumps, slopes, moving platforms, enemy stomps, powers and hidden passages. |
| [sgdk-shmup](skills/sgdk-shmup/SKILL.md) | Build vertical or horizontal shooters. Includes [Shooter Lab](skills/sgdk-shmup/assets/shooter-lab/README.md) and [Carpet Lab](skills/sgdk-shmup/assets/carpet-lab/README.md). |
| [sgdk-rpg](skills/sgdk-rpg/SKILL.md) | Build party stats, equipment, inventory, abilities, turn-based encounters and checkpoint recovery. Includes Adventure Lab. |

### Test, debug and optimize

| Skill | What to ask it for |
| --- | --- |
| [sgdk-testing](skills/sgdk-testing/SKILL.md) | Build deterministic replays, PAL/NTSC logic checks and regression reports for automated QA. Includes Test Lab. |
| [sgdk-debug](skills/sgdk-debug/SKILL.md) | Investigate a failed build, a ROM fault, a graphics problem or a slow frame. |
| [sgdk-performance](skills/sgdk-performance/SKILL.md) | Measure frame-rate headroom, compare optimizations and separate CPU, DMA and scanline limits. |
| [sgdk-memory](skills/sgdk-memory/SKILL.md) | Account for RAM, manage allocation lifetimes and diagnose fragmentation. Includes Memory Lab. |

### Ship your game

| Skill | What to ask it for |
| --- | --- |
| [sgdk-release](skills/sgdk-release/SKILL.md) | Check an SGDK ROM and package the exact tested build with documents, hashes and build records. |

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

The current installer needs Node.js 22.20 or later. Planning works without Docker.
The build workflow needs
Docker running with Linux containers. The optional helpers need Python 3.10+
and use its standard library. No native SGDK installation or API key is needed.

## Use

Ask your agent to use a skill for a concrete task. For example:

- "Use sgdk-project to create a Hello World ROM in a new folder."
- "Use sgdk-planning to plan a small platformer and identify what to prove first."
- "Use sgdk-best-practices to review this module’s arithmetic, ownership and failure handling."
- "Use sgdk-assets to import this spritesheet and check its palette budget."
- "Use sgdk-pixellab to generate palette-cohesive native sprites and validate their exports."
- "Use sgdk-graphics to animate a character over a scrolling background with a fixed HUD."
- "Use sgdk-animation to add attack phases, timed cues, interruption and one-hit-per-action hitboxes."
- "Use sgdk-effects to add explosions, weather and camera shake without crowding out gameplay sprites."
- "Use sgdk-raster to add underwater ripples and a palette switch below a moving waterline."
- "Use sgdk-audio to add music, prioritized effects and a pause-menu transition."
- "Use sgdk-input to add 3/6-button pad and mouse controls, with Team Player support."
- "Use sgdk-ui to add a title screen, options, pause menu and fixed score display."
- "Use sgdk-localization to add French and Spanish menus with accented glyphs and translated layout checks."
- "Use sgdk-dialogue to add a portrait conversation with gradual text and branching choices."
- "Use sgdk-levels to import a Tiled level with scrolling, walls and camera limits."
- "Use sgdk-streaming to preload area art with a bounded tile cache while keeping the hero and HUD resident."
- "Use sgdk-gameplay to add a scrolling player, wall collision and reusable bullets."
- "Use sgdk-shmup to build a scrolling shooter with readable bullet patterns, upgrades and a boss."
- "Use sgdk-platformer to build a responsive scrolling course with wall jumps, stomps and secrets."
- "Use sgdk-rpg to add a party, validated combat actions, status effects, equipment and battle rewards."
- "Use sgdk-math to build a fixed-point starfield and orbiting formations."
- "Use sgdk-cartridge to load stage graphics and music from separate ROM banks while keeping shared assets pinned."
- "Use sgdk-save to add checkpoint saves, a backup slot and password entry."
- "Use sgdk-memory to track RAM, diagnose a failed allocation and prove room cleanup restores the baseline."
- "Use sgdk-performance to measure a busy scene and optimize it without changing gameplay."
- "Use sgdk-debug to investigate this fault using the matching ELF file."
- "Use sgdk-testing to replay this route and catch regressions in both PAL and NTSC schedules."
- "Use sgdk-release to validate and package this release candidate without changing the ROM."

Each skill works on its own. It includes focused references and, where useful,
a helper or small example. Agents check the selected container's headers,
manuals and samples before proposing unfamiliar API calls. The default is
`ewrogers/sgdk:latest`. Pull it before building, and check the installed SDK's
version when consulting the references.

The guidance accounts for CPU work, RAM, VRAM, palettes, sprite scanlines and
transfer bandwidth separately. It helps you use SGDK directly and leaves your
game's architecture to you.

Planning includes an optional static budget checker for RAM, VRAM, palettes,
sprite peaks, padded ROM size and a declared DMA allowance. It keeps unknown
costs visible. It does not predict frame rate or certify that a game will fit.

The references also cover cartridge mapping and padding, 68000 assembly
investigations, Z80 bus ownership and FM/PSG interactions. They link to the
selected SGDK source and [Plutiedev](https://plutiedev.com/) for deeper hardware
explanations, rather than treating every build as a low-level driver project.

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

## Disclaimer

SGDK Skills is an independent community project. It is not affiliated with,
endorsed by or sponsored by SEGA CORPORATION or SEGA SAMMY HOLDINGS INC.
SEGA and related names, logos and trademarks belong to their respective owners.
They are used here to identify the hardware and tools this project supports.

This repository's original work is available under the [MIT License](LICENSE).
Third-party software and materials retain their own licenses and notices.
