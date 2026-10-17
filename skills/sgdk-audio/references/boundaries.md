# Responsibility boundaries

| Neighbor | Audio owns | Reuse from the neighbor |
| --- | --- | --- |
| `sgdk-assets` | Audible source quality, instrument/channel choices, tracker-to-driver contract | General ResComp declarations, import failures and project asset pipeline |
| `sgdk-scenes` | Music requests, fade/jingle ownership, effect lifetime policy | Scene lifecycle, loading, input handoff and resource teardown |
| `sgdk-cartridge` | Which playing music/sample bytes must remain accessible | Mapper choice, FAR mapping and SRAM coordination |
| `sgdk-input` / `sgdk-gameplay` | Event-to-cue policy and bounded retrigger behavior | The existing input snapshot, game clock and event producers |
| `sgdk-performance` / `sgdk-hardware` | Controlled audio comparisons and interpreting driver diagnostics | Full-game profiling and physical-console validation |
| `sgdk-testing` | Fake-driver command tests, authored-audio invariants and listening checklist | General replay infrastructure and regression scheduling |

Sound Desk's pure conductor is an example of one music owner and a small effect
admission policy. Its page router gives either that conductor, a direct audition
or the legacy diagnostics transport exclusive control, stopping owned playback
at page changes and waiting for pending SDK fades. Do not install all three as
competing controllers in a game. Other skills are optional companions; this
skill exports and builds independently.
