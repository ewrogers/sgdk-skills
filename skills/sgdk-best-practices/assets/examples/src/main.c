#include <genesis.h>
#include "checked.h"

/* Build/execution probe, not a playable ROM. No SDK startup or VDP behavior
 * is inferred when a test harness calls this entry point directly. */
__attribute__((noipa)) int main(bool hard_reset)
{
    (void)hard_reset;
    return bp_example_checks();
}
