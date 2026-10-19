#ifndef HARDWARE_LAB_EVERDRIVE_LINK_H
#define HARDWARE_LAB_EVERDRIVE_LINK_H
#include "hardware.h"

/* Lab-only FIFO protocol v1. Does not claim a console manufacturer. */
#define HL_ED_REQUEST 1
#define HL_ED_RESPONSE 2
#define HL_ED_ACK 3
#define HL_ED_DONE 4
#define HL_ED_PACKET_BYTES 12
#define HL_ED_XOR 0xa55a5aa5u
#define HL_ED_LEASE_SECONDS 5
typedef struct {
    u32 pending_at,confirmed_at,received_at,nonce;
    u16 hz,used;
    u8 packet[HL_ED_PACKET_BYTES];
    bool present,pending,verified,ever_verified;
} HlEdLink;
bool hl_ed_signature(u16 first,u16 second);
void hl_ed_init(HlEdLink *link,bool present,u16 hz);
void hl_ed_tick(HlEdLink *link,u32 now);
void hl_ed_packet(u8 *out,u16 kind,u32 nonce);
/* Feed bounded available bytes; a true return supplies one complete reply. */
bool hl_ed_receive(HlEdLink *link,u8 incoming,u32 now,u8 *reply);
#endif
