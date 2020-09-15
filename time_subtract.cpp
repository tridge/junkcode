#include <stdio.h>
#include <stdint.h>

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
    }
    return 0;
}
