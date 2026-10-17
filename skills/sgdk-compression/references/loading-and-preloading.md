# Loading and preloading

## Cover work that need not run during play

For a room change, fade or otherwise cover the outgoing scene, decode into owned
RAM, upload within the frame's allowance, publish the new map only after all
required transfers complete, then reveal it. A fade masks unfinished art; it does
not make decoding cheaper. Keep any input/audio service that the transition needs
running. A large synchronous decode can delay normal foreground updates even if
interrupts remain enabled.

Compression Lab fades only its picture palette. It decodes one complete 4,096-byte
packet while black, then sends 512 bytes each frame to a separate VRAM slot.
Only after the eighth transfer completes does it change the picture's tile indices
and fade in. The UI palette remains readable. This is an illustrative state
machine, not an SGDK scene or fade API requirement.

## Prepare before the asset is visible

A preload needs a deadline, retained storage and a fallback if preparation is
late. Start it far enough ahead to finish under representative foreground load.
Keep old content resident until the replacement is ready. If all the new art is
already in its private VRAM slot, showing it may only require a map/pointer switch.
If it is merely decoded in RAM, the subsequent upload still costs time.

Ordinary `unpack()` runs to completion. Calling it once per frame does **not** cap
its work. Divide large data into independently packed blocks, or deliberately
adopt a resumable decoder or task system with tested ownership. Do not stop an
ordinary compressed stream after N source bytes and start the next frame there.
Back-references and bit-reader state cross arbitrary boundaries.

The Lab's preload decodes one 512-byte-output LZ4W packet and queues that packet
per frame. The old picture and live UI remain visible. It retains a 4,096-byte
staging buffer, so this demonstrates scheduling rather than minimum staging RAM.
A one-chunk reusable buffer could reduce payload to 512 bytes, provided it is not
reused until its queued DMA has finished. Full output caching requires enough
space for all retained decoded data. Neither approach is free RAM.

## Transfers and publication

A deferred DMA queue borrows its source. Keep that source alive and immutable
until the queue drains; queue admission is not completion. On rejected admission,
retain the decoded bytes and retry without advancing progress. Keep uploaded
pieces private until the complete asset is usable. If cancellation is supported,
stop producers and wait for outstanding borrowers before releasing storage.

The Lab's loader tests rejected queues, no double decoding on retry, transfer
acknowledgement, failure and cancellation, including 1,000 repeated cancellations.
Its SDK adapter is the sole queue producer: at most 28 text rows (2,240 bytes)
plus one 512-byte art packet per frame. The benchmark's 4,096-byte picture upload
uses a separate drain. Staging and screen RAM remain valid through those drains.
Other games must include their sprites, scrolling, audio contention and other
producers in the same budget; the Lab's allowance is not a hardware promise.

## Banked cartridges and background tasks

For large ROMs, resolve a valid mapped source and retain its mapping for the
entire decode. A temporary far view is not a permanently stable pointer. Remapping
between chunks is possible only when each chunk and its dependencies are valid
under its own mapping. If preemptive background decoding is suspended mid-call,
the foreground must not remap its source/dictionary out from under it. Audio may
also need ROM access; follow that driver's bank restrictions independently.

SGDK has a user-task facility, but the Lab intentionally uses explicit chunks.
A task can consume spare frame time without rewriting a synchronous routine as a
state machine. It cannot create extra CPU time, guarantee a completion deadline,
or make shared SGDK APIs reentrant. Stack space, starvation, mapping ownership,
interrupt dependencies and safe publication need a separate design and tests.

## Primary sources

- [SGDK 2.11 DMA queue](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/dma.h)
  and [VBlank processing](https://github.com/Stephane-D/SGDK/blob/v2.11/src/sys.c)
- [Banking/far access](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/mapper.h)
- [User tasks](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/task.h)
- [Plutiedev: multitasking](https://plutiedev.com/multitasking)
