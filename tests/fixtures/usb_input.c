#ifndef USB_CHECK
#include <assert.h>
#define USB_CHECK(value) assert(value)
#endif
#ifndef UI_TEST_ENTRY
#define UI_TEST_ENTRY main
#endif
#include "usb_input.h"
#include "usb_input_ed.h"
#include "vectors.h"
static UsbInput u;
static const UiPort physical[2]={{.kind=UI_PAD,.held=4},{.kind=UI_MOUSE,.held=2,.dx=3,.dy=4}};
static UiU8 response[UI_BYTES];
static UiU8 send(const UiU8 *data,UiU32 now){
    for(UiU16 i=0;i<32;i++)ui_feed(&u,data[i],now);
    ui_step(&u,now,physical);
    for(UiU16 i=32;i<40;i++)ui_feed(&u,data[i],now+1);
    ui_step(&u,now+1,physical);
    return ui_reply(&u,response);
}
int UI_TEST_ENTRY(void){
    USB_CHECK(sizeof(UsbInput)<=128 && sizeof(UiPort)==12);
    ui_init(&u,0,60);
    USB_CHECK(!send(open_session,0));ui_ed_poll(&u,3,0,0);ui_ed_finish(&u,3,physical);
    USB_CHECK(u.port[0].held==4 && !u.port[0].remote);
    ui_init(&u,1,60);
    USB_CHECK(send(two_ports,0) && response[36]==UI_BAD_SESSION);
    USB_CHECK(send(open_session,2) && response[36]==UI_OK);
    USB_CHECK(send(two_ports,10) && response[36]==UI_OK && response[5]==3);
    USB_CHECK(u.port[0].held==0x50 && u.port[0].pressed==0x50 && u.port[0].released==4);
    USB_CHECK(u.port[1].kind==UI_MOUSE && u.port[1].dx==20 && u.port[1].dy==-10 && u.port[1].held==5);
    USB_CHECK(response[20]==0 && response[21]==0 && response[28]==0 && response[29]==20);
    ui_step(&u,12,physical);
    USB_CHECK(u.port[1].dx==0 && u.port[1].dy==0 && u.port[1].held==5);
    ui_step(&u,13,physical);USB_CHECK(u.port[0].remote);
    ui_step(&u,14,physical);
    USB_CHECK(!u.port[0].remote && u.port[0].held==4 && u.port[0].pressed==4 && u.port[0].released==0x50);
    USB_CHECK(!u.port[1].remote && u.port[1].dx==3 && u.port[1].dy==4 && u.port[1].held==2);
    USB_CHECK(send(two_ports,15) && response[36]==UI_BAD_SEQUENCE && !u.port[1].remote);
    USB_CHECK(!send(corrupt,17) && !u.port[1].remote);
    USB_CHECK(send(invalid_second,20) && response[36]==UI_BAD_ARGUMENT && !u.port[0].remote);
    USB_CHECK(send(mouse_one,22) && response[36]==UI_OK);
    USB_CHECK(u.port[0].kind==UI_MOUSE && u.port[0].dx==12 && u.port[0].dy==8 && u.port[0].pressed==1);
    USB_CHECK(!u.port[1].remote && u.port[1].held==2);
    USB_CHECK(send(release_one,24) && response[36]==UI_OK && !u.port[0].remote);
    USB_CHECK(u.port[0].kind==UI_PAD && u.port[0].held==4 && u.port[0].released==1);
    USB_CHECK(send(wrong_session,26) && response[36]==UI_BAD_SESSION);
    USB_CHECK(send(status_request,28) && response[36]==UI_OK && response[5]==0);
    UiU16 legacy=0;
    for(UiU16 i=0;i<12;i++)legacy+=ui_feed(&u,legacy_link[i],30);
    USB_CHECK(legacy==1 && !u.ack_pending);
    for(UiU16 i=0;i<12;i++)USB_CHECK(u.wire[i]==legacy_link[i]);
    USB_CHECK(send(long_both,40) && response[36]==UI_OK);
    USB_CHECK(send(release_first,42) && response[36]==UI_OK && response[5]==2);
    USB_CHECK(u.port[1].remote && u.port[1].held==5 && !u.port[1].dx);
    USB_CHECK(send(only_first_pad,44) && response[36]==UI_OK && response[5]==3);
    USB_CHECK(send(release_second,46) && response[36]==UI_OK && response[5]==1);
    USB_CHECK(u.port[0].remote && u.port[0].held==0x10 && !u.port[1].remote);
    USB_CHECK(send(mouse_start,48) && response[36]==UI_OK && u.port[1].held==15);
    USB_CHECK(send(sequence_max,50) && response[36]==UI_OK);
    USB_CHECK(send(two_ports,52) && response[36]==UI_BAD_SEQUENCE);
    ui_init(&u,1,50);
    for(UiU16 i=0;i<10;i++)ui_feed(&u,open_session[i],0);
    for(UiU16 i=10;i<40;i++)ui_feed(&u,open_session[i],51);
    USB_CHECK(!u.ack_pending && !u.session);
    USB_CHECK(send(open_session,52) && response[36]==UI_OK);
    USB_CHECK(send(two_ports,0xfffffffeu) && response[36]==UI_OK);
    ui_step(&u,0,physical);USB_CHECK(u.port[0].remote && !u.port[1].dx);
    ui_step(&u,1,physical);USB_CHECK(u.port[0].remote);
    ui_step(&u,2,physical);USB_CHECK(!u.port[0].remote);
    return 0;
}
