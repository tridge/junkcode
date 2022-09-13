#include <string.h>
#include <stdio.h>
#include <stdint.h>

/*
  simple base64 decoder, not particularly efficient, but small
 */
static int32_t base64_decode(const char *s, uint8_t *out, const uint32_t max_len)
{
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    const char *p;
    uint32_t n = 0;
    uint32_t i = 0;
    while (*s && (p=strchr(b64,*s))) {
        const uint8_t idx = (p - b64);
        const uint32_t byte_offset = (i*6)/8;
        const uint32_t bit_offset = (i*6)%8;
        out[byte_offset] &= ~((1<<(8-bit_offset))-1);
        if (bit_offset < 3) {
            if (byte_offset >= max_len) {
                break;
            }
            out[byte_offset] |= (idx << (2-bit_offset));
            n = byte_offset+1;
        } else {
            if (byte_offset >= max_len) {
                break;
            }
            out[byte_offset] |= (idx >> (bit_offset-2));
            n = byte_offset+1;
            if (byte_offset+1 >= max_len) {
                break;
            }
            out[byte_offset+1] = (idx << (8-(bit_offset-2))) & 0xFF;
            n = byte_offset+2;
        }
        s++; i++;
    }

    if ((n > 0) && (*s == '=')) {
        n -= 1;
    }

    return n;
}

int main(void)
{
    const char *b = "WdlOdutZz+Gb4AGFptgsnXXAgfdR0l+K4vR7TmKawuw=";
    uint8_t out[32];
    int32_t out_len = base64_decode(b, out, sizeof(out));
    printf("out_len %u\n", out_len);
    return 0;
}
