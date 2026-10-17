# SDK execution and stacks

Verified against SGDK 2.11 in `ewrogers/sgdk:latest`, GCC 16.2.0 and ResComp 3.95.
Recheck the selected image before adapting low-level code.

## One CPU, two contexts

The foreground runs in supervisor mode. SGDK can run **one** configured user task
while the foreground waits. VBlank preempts it and resumes the supervisor. It is
not an operating system, a thread pool, extra processor or general synchronization
library. Background jobs share one callback; dispatch them in that callback.

The selected SDK exposes these signatures in `task.h`:

| API | Role |
| --- | --- |
| `void TSK_init(void)` | Initialize task state; use during setup, not while preserving a continuation. |
| `void TSK_userSet(VoidCallback *task)` | Set the saved user PC. It does not reset the user stack or all registers. |
| `void TSK_stop(void)` | Clear the saved user PC. It is not a resumable pause. |
| `void TSK_userYield(void)` | Give the user task time until a switch returns to the supervisor. |
| `bool TSK_superPend(s16 wait)` | Run the task while the supervisor waits for a post or timeout; positive waits count frames. |
| `void TSK_superPost(bool immediate)` | User task posts to the waiting supervisor; immediate uses trap 0, otherwise returns on VBlank. |

For a positive wait, `TSK_superPend` returns false on a post, true on timeout.
The Lab does not depend on indefinite waits or post/pending APIs. Do not introduce
those into a frame wait casually: an immediate post may resume foreground code
**before VBlank**, making an assumed-safe DMA/VDP service point unsafe.

`SYS_doVBlankProcess()` already reaches task-aware waits in `vdp.c`. Use the normal
frame loop; do not add a second yield to “speed it up.” Interrupts must be enabled.
Keep callbacks non-returning; the SDK does not establish a normal caller return
address as a task lifetime mechanism.

## Stack and interrupt policy

Startup reserves 512 bytes for the user stack within the SDK's normal stack
reservation (2,560 bytes in this image). C decoding and nested calls need an
explicit stack audit. Lab `stack.s` is a supervisor-only helper that sets USP to a
separate static 2,048-byte buffer. Its bottom 64 bytes are a checked guard. This
buffer is **additional RAM**, not a reduction of the existing SDK reservation.

Fill patterns estimate bytes touched. A surviving guard does not prove that all
paths fit, nor detect every out-of-bounds write. Test optimized/debug code, codec
worst cases and nested calls; inspect stack usage and leave headroom. An overflow
may corrupt memory before the foreground can report it.

The SDK initializes user SR to `0x0400`: user mode with interrupt mask 4. VBlank
(level 6) can preempt; ordinary HBlank (level 4) is masked while the task runs.
**Do not assume compatibility with raster effects.** Lowering that mask is a
separate low-level design decision requiring handler, stack and timing validation.
Never call privileged interrupt-control routines from user mode.

Lab holds work by waiting for an actual new VBlank in the foreground without
calling a task-aware wait, then servicing `SYS_doVBlankProcessEx(IMMEDIATELY)`.
This preserves the saved continuation. IMMEDIATELY is safe here only because that
preceding wait establishes the service point; it is not a general frame-loop fix.

## Primary sources

- [SGDK v2.11 task header](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/task.h)
- [Task implementation](https://github.com/Stephane-D/SGDK/blob/v2.11/src/task.s)
- [Startup, VBlank and trap handlers](https://github.com/Stephane-D/SGDK/blob/v2.11/src/boot/sega.s)
- [Task-aware VDP waits](https://github.com/Stephane-D/SGDK/blob/v2.11/src/vdp.c)
- [Task constants](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/task_cst.h)
- [Memory reservation](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/memory_base.h)
- [Plutiedev: multitasking](https://plutiedev.com/multitasking) explains the underlying user/supervisor technique; verify SDK behavior in its own source.
