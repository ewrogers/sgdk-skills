# Atomic grid rules and undo

## Clockwork Garden's rule contract

The 12×9 board is row-major; screen pixels are derived later. Terrain is immutable:
floor, wall, diamond seal, square pressure plate, gate, gap and exit. The player,
up to four lantern crates, filled-gap bits and move count form one 8-byte state.
`255` means a consumed/unused crate. Gaps receive stable indices at parse time.

One directional command moves the player one cell or pushes one crate one cell.
A wall, closed gate, another crate or boundary blocks the complete operation.
Walking into an unfilled gap is blocked. Pushing a crate into a gap consumes it
and fills that gap in the same transaction. Chain pushing is deliberately absent.

Any occupied pressure plate opens all gates in this small example. An occupied
gate also holds gates open, so a closing door cannot trap an occupant inside
solid terrain. After it is empty, gates close unless a plate remains occupied.
This safety hold is a game policy, not a general pressure-plate convention.
Use identified circuits if the game has independent doors; bound their links
and define whether multiple plates mean AND, OR, toggling or a timed latch.

Cover every seal to light the exit. Completion requires the player to enter it.
The exit remains walkable while unlit, but does not complete the room. A completed
board accepts undo; other movement is blocked until the adapter advances rooms.

## Validate, stage, commit

`pz_move` checks bounds in x/y before forming a flat index, preventing row wrap.
It copies the small state, validates the destination and any pushed destination,
then changes that candidate. Only a legal move enters the history ring and
replaces live state. Trigger state is derived from the snapshot; there is no
separate door cache to restore. Invalid direction, blocked push and post-clear
movement leave the entire board/history unchanged.

`pz_parse` similarly stages a board before replacing one. It rejects the wrong
size, unknown cells, duplicate/missing player, missing/duplicate exit, too many
crates and more than eight gaps. This is format validation, not proof of
solvability. Keep a separate validator/search for authored puzzles and distinguish
an unsolvable layout from a legal move that creates a deadlock.

## Undo and animation

The 64-entry ring stores the state *before* each legal move. Full history evicts
the oldest entry. Undo restores player, every crate, filled gaps and move count
together. Restart restores the room's initial snapshot and clears history.
The counter saturates at 65535 rather than wrapping. History indexes, pointer
padding, sprites and sound playback do not belong in serialized game state.

The Lab commits immediately, then interpolates positions for six 60-Hz ticks.
Movement waits during that interpolation. A fresh A undoes and snaps to the
restored board; B restarts and snaps. Pause freezes interpolation and repeat.
Room-clear presentation waits until the last move reaches its visual endpoint.

Other games may buffer one direction, animate before commit or allow undo after
completion differently. Keep those decisions explicit and test interruptions.

## Evidence

The C contracts prove blocked-operation immutability, pushes, bridge consumption
and undo, plate/gate transitions, parser failure, ring eviction and restart.
An independent Python set-based solver searches all three rooms to a solution;
7,200 seeded commands, including undo/restart, compare actual C state against
that oracle at both host optimization levels. A further 20,000-step C soak checks
invariants. The target probe runs the same contracts and a shorter deterministic
trace on a base-M68000 model. None of those tests checks visual interpolation.
