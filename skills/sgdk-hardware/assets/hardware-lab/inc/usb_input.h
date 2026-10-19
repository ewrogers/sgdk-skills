#ifndef USB_INPUT_H
#define USB_INPUT_H
/* The bare-metal images have no hosted libc stdint.h. Compiler-provided
 * exact-width types also keep this source independent of either SDK. */
typedef __UINT8_TYPE__ UiU8;
typedef __UINT16_TYPE__ UiU16;
typedef __UINT32_TYPE__ UiU32;
typedef __INT16_TYPE__ UiS16;

/* Optional logical input, never an emulation of electrical controller ports. */
#define UI_BYTES 40
#define UI_PHYSICAL 0
#define UI_PAD 1
#define UI_MOUSE 2
#define UI_MAX_TICKS 120
#define UI_OPEN 1
#define UI_STATE 2
#define UI_RELEASE 3
#define UI_STATUS 4
#define UI_ACK 128
#define UI_OK 0
#define UI_BAD_COMMAND 1
#define UI_BAD_SESSION 2
#define UI_BAD_SEQUENCE 3
#define UI_BAD_ARGUMENT 4

typedef struct {
    UiU16 held,pressed,released;
    UiS16 dx,dy;
    UiU8 kind,remote;
} UiPort;
typedef struct {
    UiU32 since;
    UiU16 ttl,held;
    UiS16 dx,dy;
    UiU8 kind;
} UiOverride;
typedef struct {
    UiPort port[2];
    UiOverride override[2];
    UiU32 session,sequence,received_at,ack_session,ack_sequence,tick;
    UiU16 used,hz;
    UiU8 wire[UI_BYTES];
    UiU8 present,ack_pending,ack_code;
} UsbInput;

void ui_init(UsbInput *input,UiU8 everdrive_present,UiU16 hz);
/* One FIFO owner: returns 1 for a complete legacy HLV1 packet in wire[0..11].
 * UIV1 commands are processed internally. At most 32 bytes may be fed/frame. */
UiU8 ui_feed(UsbInput *input,UiU8 byte,UiU32 now);
/* Call exactly once at the game's existing input-sampling boundary. Physical
 * values come from its current owner. Returned port[] is immutable this frame. */
void ui_step(UsbInput *input,UiU32 now,const UiPort physical[2]);
/* After ui_step: one ACK with the actual sampled logical states, or zero.
 * ACK means sampled by this hook, not consumed by gameplay or seen on HDMI. */
UiU8 ui_reply(UsbInput *input,UiU8 out[UI_BYTES]);
#endif
