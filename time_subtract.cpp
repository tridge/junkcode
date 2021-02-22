#include <stdio.h>
#include <stdint.h>

/*
  subtracting two 16 bit timestamps must cast the result to uint16_t
  or it does not handle wrap correctly. Using this inline function
  produces the correct result
 */
static inline uint16_t time_diff16(uint16_t curr_t, uint16_t prev_t)
{
    return uint16_t(curr_t - prev_t);
}

/*
  subtracting two 16 bit timestamps must cast the result to uint16_t
  or it does not handle wrap correctly. Using this inline function
  produces the correct result
 */
static inline uint32_t time_diff32(uint32_t curr_t, uint32_t prev_t)
{
    uint32_t delta = curr_t - prev_t;
    if (delta > 0xF0000000U) {
        // very likely a bug, subtracting a newer timestamp from an
        // older one
    }
    return delta;
}



int main(void)
{
    uint16_t x1 = 0;
    uint16_t x2 = 10;
    for (uint32_t i=0; i<0x20000; i++) {
        x1++;
        x2++;
        if (x2 - x1 != 10) {
            printf("bad wrap1\n");
        }
        if (x2 - x1 != uint16_t(10)) {
            printf("bad wrap2\n");
        }
        if (uint16_t(x2 - x1) != 10) {
            printf("bad wrap3\n");
        }
        if (time_diff16(x2, x1) != 10) {
            printf("bad wrap4\n");
        }
    }
    return 0;
}
