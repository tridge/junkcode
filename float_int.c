#include <stdio.h>
#include <math.h>
#include <stdint.h>

int main(void)
{
    for (int32_t i=0; i>-0x3FFFFFF; i--) {
        float f = i;
        int32_t i2 = (int32_t)f;
        if (i2 != i) {
	    printf("Max- 0x%x %d %f\n", i+1, i2, f);
            break;
        }
    }
    for (int32_t i=0; i<0x3FFFFFF; i++) {
        float f = i;
        int32_t i2 = (int32_t)f;
        if (i2 != i) {
	    printf("Max+ 0x%x %d %f\n", i-11, i2, f);
            break;
        }
    }
    uint32_t mask = 0;
    for (int8_t i=0; i<32; i++) {
	mask |= (1U<<i);
	float f = mask;
	uint32_t i2 = (uint32_t)f;
	if (i2 != mask) {
	    printf("MaxBit %u\n", (unsigned)(i-1));
            break;
        }
    }
    return 0;
}
