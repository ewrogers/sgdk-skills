#include "checked.h"
#define CHECK(value) do { if (!(value)) return __LINE__; } while (0)

int bp_example_checks(void)
{
    CHECK(bp_square(65535u) == 4294836225UL);
    CHECK(bp_add_clamped(65530u, 10u) == 65535u);
    CHECK(bp_add_clamped(10u, 20u) == 30u);
    CHECK(bp_elapsed(3, 65534u) == 5);
    u16 mask = 123;
    CHECK(bp_layer(15, &mask) && mask == 32768u);
    CHECK(!bp_layer(16, &mask) && mask == 32768u);
    CHECK(!bp_layer(65535u, &mask) && !bp_layer(0, 0));
    const u8 bytes[9] = {0, 1, 7, 3, 232, 255, 128, 0, 1};
    BpRecord record = {0, 0, 0};
    /* Byte-offset source: never reinterpret it as an aligned struct/u32. */
    CHECK(bp_decode(bytes + 1, 8, &record));
    CHECK(record.score == 4286578689UL && record.health == 1000 && record.flags == 7);
    u8 encoded[9] = {0};
    CHECK(bp_encode(&record, encoded + 1, 8));
    for (u16 i = 1; i < 9; i++) CHECK(encoded[i] == bytes[i]);
    CHECK(!bp_decode(bytes + 1, 7, &record) && record.health == 1000);
    encoded[1] = 2;
    CHECK(!bp_decode(encoded + 1, 8, &record) && record.score == 4286578689UL);
    return 0;
}

#ifdef BP_HOST_MAIN
#include <stdio.h>
int main(void)
{
    const int result = bp_example_checks();
    if (result) { printf("Check failed at line %d\n", result); return 1; }
    puts("C contracts OK");
    return 0;
}
#endif
