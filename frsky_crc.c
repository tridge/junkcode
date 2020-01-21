/*
  compare two possible FrSky CRC implementations
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>



static uint8_t crc1(uint8_t len, const uint8_t *b)
{
    uint16_t sum = 0;
    for (uint8_t i=0; i<len; i++) {
        sum += b[i];
        sum += sum >> 8;
        sum &= 0xFF;
    }
    sum = (sum & 0xff) + (sum >> 8);
    return sum;
}

static uint8_t crc2(uint8_t len, const uint8_t *b)
{
    uint16_t sum = 0;
    for (uint8_t i=0; i<len; i++) {
        sum += b[i];
    }
    sum = (sum & 0xff) + (sum >> 8);
    return sum;
}

int main(void)
{
    const uint8_t len = 25;
    uint8_t buf[len];
    while (true) {
        for (uint16_t i=0; i<len; i++) {
            buf[i] = (uint8_t)random();
        }
        uint8_t sum1 = crc1(len, buf);
        uint8_t sum2 = crc2(len, buf);
        if (sum1 != sum2) {
            printf("ERR: %02x %02x\n", sum1, sum2);
        } else {
            printf("OK:  %02x %02x\n", sum1, sum2);
        }
    }
    return 0;
}
