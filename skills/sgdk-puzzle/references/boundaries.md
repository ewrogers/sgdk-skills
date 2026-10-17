# Responsibility boundaries

Use the project's existing implementation at each seam. These skills are optional
companions, not installed dependencies of Puzzle Lab.

| Area | Puzzle owns | Delegate or reuse |
| --- | --- | --- |
| `sgdk-levels` | Logical cell meaning and authored board validation | Tiled import, map units, scrolling and MAP streaming |
| `sgdk-collision` | Discrete destination/footprint legality and push rules | Sweeps, rays, continuous contacts and sliding |
| `sgdk-gameplay` / `sgdk-input` | Turn acceptance and puzzle-specific repeat/drop policy | Established clock, controller snapshots, action bindings and ownership |
| `sgdk-animation` | Which committed move starts which presentation | Animation event clocks and interruption already in the game |
| `sgdk-testing` | Solution traces, board invariants and puzzle regression cases | General replay recording, QA scheduling and checkpoint infrastructure |
| `sgdk-save` | Which logical fields would need persistence | Versioned save bytes, SRAM, corruption recovery and migration |
| `sgdk-ui` / `sgdk-graphics` | Board feedback and logical-to-screen mapping | Fonts, menus, plane/sprite helpers and palette ownership |
| `sgdk-memory` / `sgdk-performance` | Explicit state/history/search bounds | Whole-game RAM accounting and measured optimization |

The two Labs use their own small adapters because each exported project must run
alone. They are not recommendations to install a second clock, input reader or
rendering system into an existing game. Their only common dependency is SGDK;
portable rule modules contain no VDP, pad or sound calls.
