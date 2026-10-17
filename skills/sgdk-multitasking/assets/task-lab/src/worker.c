#include "platform.h"
MTWork mt_work;
u16 mt_stack[MT_STACK_WORDS];
static void worker(void) {
    /* Callback never returns. Only flags/private RAM and the pure decoder here. */
    while(TRUE)mt_step(&mt_work,mt_decode,0);
}
void mt_start_worker(void) {
    /* Called once from supervisor, before the task can run. */
    for(u16 i=0;i<MT_STACK_WORDS;i++)mt_stack[i]=0xA55A;
    mt_init(&mt_work);TSK_init();
    mt_set_user_stack(mt_stack+MT_STACK_WORDS);
    TSK_userSet(worker);
}
