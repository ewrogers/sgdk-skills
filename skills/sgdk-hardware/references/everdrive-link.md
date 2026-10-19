# Hardware Lab EverDrive/USB check

Hardware Lab owns the optional guest diagnostic and its small versioned wire
format. sgdk-project owns serial identity, deployment and `verify-lab`; ordinary
ROM deployment still requires no guest hooks. The Lab is independently buildable
without sgdk-project. Using the host exchange additionally requires its installed
USB client and pySerial. No console manufacturer or emulator classification is
inferred. This is a diagnostic example, not a required game framework.

## Detection and UI

Read two volatile 16-bit status words at `$A130D4`. Both must match `$55A0` under
mask `$FFF0`, with bit 3 changing between reads. This recognizes the published
EDIO signature and read strobe, not every EverDrive model or authenticated
physical hardware. The interface exists in PRO/CORE sources; the current host
client separately requires a verified Pro identity. If absent, skip all FIFO
access and show EVERDRIVE EDIO UNKNOWN / USB LINK UNAVAILABLE.

With EDIO present, the screen starts NOT RUN. It becomes PASS/LIVE only after
receiving a matching acknowledgement for a fresh request. A five-second lease
using the existing `vtimer` and actual PAL/NTSC flag supplies recent bidirectional
liveness. After renewal stops, PASS/IDLE preserves the earlier successful check
without claiming a current host session. It is not an electrical cable-presence
sensor. The game keeps polling on every page; C resets counters without affecting
leases. Both reset entry paths explicitly initialize all protocol state.

## Optional host command

Run the compatible Hardware Lab, then use the installed sgdk-project helper:

```sh
python /path/to/sgdk-project/scripts/everdrive.py --port ACTUAL_PORT --timeout 40 verify-lab --duration 30
```

`verify-lab` first verifies the Pro identity. Duration zero (default) performs one
exchange; 0-300 seconds is accepted for a bounded renewal run. Allow sufficient
overall `--timeout` for that run. Renewals pause up to one second after each
completed exchange; ordinary transport/response deadlines still apply. It prints
JSON including `lab_link_verified`, completed handshake count, five-second lease
and unknown console model. An absent/wrong ROM, stale/malformed reply,
disconnection or deadline failure exits nonzero. Only one process owns the port;
do not deploy simultaneously. Target this command only at the compatible Lab,
because it writes diagnostic packets into the running guest's FIFO.

The command does not reset, upload, write saves or modify cartridge configuration.
Stopping renewal lets the live lease expire; the screen retains PASS/IDLE. A successful host check does
not replace HDMI observation of the build ID, boot values and running indicator.

## Lab protocol v1 and budget

Each 12-byte packet is `HLV1` (four ASCII bytes), kind (big-endian u16), nonce
(big-endian u32), then CRC-16/CCITT over the first ten bytes (polynomial `$1021`,
initial `$FFFF`, no final XOR). Kind 1 requests, 2 responds, 3 acknowledges,
4 confirms completion. The host chooses a fresh nonzero 32-bit nonce; the guest
responds with nonce XOR `$A55A5AA5`; host acknowledges the original nonce; guest
marks its lease and returns completion with that nonce. The host succeeds only
after reading the matching completion. This is not a security/authentication scheme.

Host-to-guest uses the existing FCI FIFO memory-write command at `$01810000`.
Guest-to-host uses Krikzz CMD_USB_WR (`$22`), u16 payload length, then packet.
Guest byte I/O is at `$A130D1`, available-byte count at `$A130D2 & $7FF`.
There are no game-side busy waits, disk commands, DMA acknowledgements or FIFO
flush at startup. Each frame consumes at most 32 available bytes, with at most
three completed packets / 54 outbound command bytes in that bound. Normal
exchange is two incoming 12-byte packets and two outgoing 18-byte commands,
spread across frames. Pending requests and incomplete packets expire after one
second. Malformed CRCs, unknown kinds, zero requests and unmatched ACKs cannot
verify a link. Parsing and lease logic use a fixed-size state object (target
probe checks at most 48 bytes), 12-byte reply scratch and bounded CRC work.
These are work/storage bounds, not a measured 68000 cycle allowance.

The screen adds one padded 11-character DMA-copy field at its existing ten-loop
refresh cadence; tile art and its VRAM allocation are unchanged. Check total
frame work before reusing this diagnostic in a game.

## Primary contracts and tested boundary

- [Krikzz guest register layout](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/edio/everdrive.h)
  and [FPGA status/strobe](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/fpga/mapper/lib_base/base_io.sv).
- [Krikzz guest USB/FIFO implementation](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/edio/everdrive.c):
  `ed_cmd_usb_wr`, `ed_fifo_wr`, `ed_fifo_rd`; the Lab supplies bounded polling
  instead of copying blocking receive loops.
- [Official host FifoWR/MemWR](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Device/DeviceIO_V1.cs).
- [SGDK 2.11 system API](https://github.com/Stephane-D/SGDK/blob/v2.11/inc/sys.h):
  existing VBlank service and video flag; the Lab adds no second scheduling clock.

On one Pro with MCU `$0112`, the newer FCI mailbox read returned `$FFFF`; the
first mailbox attempt timed out and did not verify. The shipped path uses
established FIFO/USB commands and was observed through an HD60 X: WAITING,
VERIFIED during nine exchanges over ten seconds, then EXPIRED after renewal
stopped, with a moving heartbeat and MISS zero in the inspected frames.
This does not establish compatibility with every mapper/firmware or USB handling
under sustained stress. No console model was assumed or automatically detected.

The clearer status follow-up was observed on 2026-10-19 after the user's firmware
update (extended protocol discovery): build `HL-C3F86D33-RA`, ROM SHA-256
`9bc6e872792d86cfeb7b044cb11ee93dd8ff56175165db8ae7b817a08d5d19e4`.
Direct HD60 X frames showed NOT RUN, then PASS/LIVE during six fresh exchanges
over six seconds, then PASS/IDLE after renewal stopped. Boot was HARD, DATA 1234,
BSS 0000, all startup checks PASS, with advancing counters/heartbeat and MISS zero
in the inspected frames. The live lease still expires after five seconds;
PASS/IDLE records the previous success rather than claiming current connectivity.
This release build used the freshly pulled `ewrogers/sgdk:latest` (SGDK 2.11,
GCC 16.2.0, binutils 2.47.20260726). The guest wire protocol and timeout behavior
are unchanged.
