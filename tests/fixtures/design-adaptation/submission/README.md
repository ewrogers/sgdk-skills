# Last Service

A 90-second tram dispatch shift in an ordinary rainy city, 1998. Open
`release/last-service.bin` in a Mega Drive / Genesis emulator. Use controller 1.

- Title/report: Up/Down chooses; **C** selects.
- Desk: Up/Down selects a waiting car; Left/Right selects a bay.
- **C dispatches**, **B clears the bay**, **A holds/runs the clock**.
- **Start** opens/closes the desk guide and freezes simulation. Closing restores
  the prior running/held state. While held, selection and dispatch still work.

X and Y are shared interlocks. Amber rails/lamps indicate occupancy; the four
route choices turn red and say LOCK when unavailable for the selected car.
A tram holds its bay and interlocks for six seconds. Its countdown appears at
its bay. North needs X, Market needs Y, Canal needs both; bays 1–2 need X and
bays 3–4 need Y. A route reserves the union. Two disjoint routes can run at once.

The desk admits three waiting cars. Due cars beyond that stay in FIFO backlog,
which is shown and included in waiting time and the final report. Waiting time
is summed across all due, undispatched services, including backlog. The display
rounds down to whole service-seconds. Transit is not waiting. All 18 services
arrive by 85 seconds. Retry repeats the same timetable.

## Build and tests

From this directory, with Docker available:

```sh
./build.sh
./tests/run.sh
python3 tools/verify_rom.py out/rom.bin
```

The Docker build uses `ewrogers/sgdk:latest` and `/opt/sgdk/makefile.gen` on the
host's native architecture. The candidate used image digest
`sha256:1d8dd3f289ef9ed1c7186b68b47a1977bc02b2bcbd72903e7682f6b19ce7d0df`.
The output is `out/rom.bin`; the submitted copy is `release/last-service.bin`.
For the same toolchain bytes, replace `:latest` in build.sh with that digest.
Host tests require a C99 compiler; ROM verification uses Python standard library.

Tests cover the actual `src/desk.c` compiled into the ROM. They prove route
conflicts, release/reuse, saturation/backlog, hold/help, retry and region timing.
The ROM build and checksum/header inspection pass. No emulator or hardware
playtest has been run. Controller feel, legibility, tram motion, sound level,
frame cost and real hardware timing still need playtesting; see ENGINEERING.md.
