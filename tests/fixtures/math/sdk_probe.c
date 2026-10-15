#include <genesis.h>

/* Entered directly by the target-code harness, without console startup.
 * These are actual SGDK routines and tables, not host replacements. */
__attribute__((noipa)) int main(bool hard_reset)
{
    (void)hard_reset;
    if (F16_mul(FIX16(-1.5), FIX16(0.5)) != FIX16(-0.75)) return 1;
    if (F16_div(FIX16(3), FIX16(2)) != FIX16(1.5)) return 2;
    if (F16_toInt(FIX16(-1.25)) != -2) return 3;
    if (F16_toRoundedInt(FIX16(-1.5)) != -2) return 4;
    if (F16_normalizeAngle(FIX16(-450)) != FIX16(270)) return 5;
    if (F16_cos(F16_normalizeAngle(FIX16(450))) != FIX16(0)) return 6;
    if (F32_mul(FIX32(1.5), FIX32(2)) != FIX32(3)) return 7;
    if (F32_mul(1, FIX32(1)) != 0) return 8; /* SDK discards low input bits. */
    if (F32_div(FIX32(1), FIX32(0.125)) != FIX32(8)) return 9;
    if (F32_sin(FIX16(90)) != FIX32(1)) return 10;
    if (F16_toRoundedInt(FIX16(1.5)) != 1) return 11;
    if (F16_toRoundedInt(FIX16(1.75)) != 2) return 12;
    if (muls(-123, 1024) != -125952) return 13;
    if (muls(-32768, -32768) != 1073741824L) return 14;
    vu16 *port = (vu16 *)VDP_DATA_PORT;
    for (u16 i = 0; i < 360; i++) *port = F16_sin(FIX16(i));
    return 0;
}
