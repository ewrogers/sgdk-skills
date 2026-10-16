#include <genesis.h>
#include "checks.h"
/* Internal SDK startup entry, used only because this CPU probe bypasses boot. */
extern void MEM_init(void);

__attribute__((noipa,used,externally_visible))
int main(bool hard_reset)
{
    (void)hard_reset;
    MEM_init();
    u16 free_before=MEM_getFree();
    if(MEM_alloc(0))return 1000;
    void *odd=MEM_alloc(1);if(!odd || free_before-MEM_getFree()!=4)return 1001;
    MEM_free(odd);MEM_free(NULL);MEM_pack();
    void *hole=MEM_alloc(3072),*barrier=MEM_alloc(256);
    if(!hole || !barrier)return 1003;
    MEM_free(hole); /* Match SDK startup leaving free space before a live allocation. */
    int error=check_fragments();if(error)return error;
    error=check_lifetimes();if(error)return error;
    error=check_soak(32);if(error)return error;
    MEM_free(barrier);MEM_pack();
    volatile u16 *port=(volatile u16 *)0xC00000;
    *port=sizeof(MemoryLab);*port=STACK_SIZE;*port=MEM_getFree();*port=MEM_getAllocated();
    *port=ML_BLOCK_BYTES;*port=ML_STAGE;
    return MEM_getFree()==free_before ? 0 : 1002;
}
