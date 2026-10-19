#ifndef USB_INPUT_ED_H
#define USB_INPUT_ED_H
#include "usb_input.h"

/* Main CPU only. The caller owns the existing frame/input and FIFO boundaries. */
static inline UiU8 ui_ed_detect(void){
    volatile UiU16 *status=(volatile UiU16 *)0xa130d4;
    UiU16 a=*status,b=*status;
    return (a&0xfff0)==0x55a0 && (b&0xfff0)==0x55a0 && ((a^b)&8)!=0;
}
static inline void ui_ed_send(const UiU8 *data,UiU16 length){
    volatile UiU8 *fifo=(volatile UiU8 *)0xa130d1;
    *fifo=0x2b;*fifo=0xd4;*fifo=0x22;*fifo=0xdd;*fifo=(UiU8)(length>>8);*fifo=(UiU8)length;
    for(UiU16 i=0;i<length;i++)*fifo=data[i];
}
/* 32 incoming bytes/frame; at most one input ACK plus two legacy replies:
 * <=82 outgoing command bytes. No wait, flush, disk initialization or ISR. */
static inline void ui_ed_poll(UsbInput *u,UiU32 now,void (*legacy)(void *,const UiU8 *,UiU32),void *ctx){
    if(!u->present)return;
    UiU16 count=*(volatile UiU16 *)0xa130d2&0x7ff;if(count>32)count=32;
    for(UiU16 i=0;i<count;i++)if(ui_feed(u,*(volatile UiU8 *)0xa130d1,now) && legacy)legacy(ctx,u->wire,now);
}
static inline void ui_ed_finish(UsbInput *u,UiU32 now,const UiPort physical[2]){
    ui_step(u,now,physical);
    UiU8 reply[UI_BYTES];if(ui_reply(u,reply))ui_ed_send(reply,UI_BYTES);
}
#endif
