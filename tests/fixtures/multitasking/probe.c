#include "platform.h"
static void emit(u16 v){*(volatile u16 *)0xC00000=v;}
__attribute__((noipa,used,externally_visible))
int main(bool hard_reset) {
    (void)hard_reset;
    mt_init(&mt_work);
    for(u16 kind=0;kind<2;kind++) {
        if(!mt_request(&mt_work,kind,1))return 1;
        u16 steps=0;
        while(mt_work.state!=MT_READY && steps<200) {
            steps++;mt_step(&mt_work,mt_decode,0);
        }
        if(mt_work.state!=MT_READY)return 2;
        emit(steps);
        for(u16 i=0;i<MT_BYTES/2;i++)emit(mt_work.output[i]);
        if(!mt_cancel(&mt_work))return 3;
    }
    if(!mt_request(&mt_work,MT_RADAR,0))return 4;
    mt_step(&mt_work,mt_decode,0);
    if(!mt_cancel(&mt_work) || mt_request(&mt_work,MT_DECODE,0))return 5;
    mt_step(&mt_work,mt_decode,0);
    if(!mt_acknowledge(&mt_work))return 6;
    emit(sizeof(MTWork));return 0;
}
