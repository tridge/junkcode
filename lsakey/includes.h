#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

typedef unsigned int uint32;
typedef unsigned char uchar;
typedef unsigned char uint8_t;

#include "md5.h"

#define ZERO_STRUCT(x) memset((char *)&(x), 0, sizeof(x))

typedef struct 
{
        struct MD5Context ctx;
        uchar k_ipad[65];    
        uchar k_opad[65];

} HMACMD5Context;

void hmac_md5_init_rfc2104(const uchar*  key, int key_len, HMACMD5Context *ctx);
void hmac_md5_init_limK_to_64(const uchar* key, int key_len,HMACMD5Context *ctx);
void hmac_md5_update(const uchar* text, int text_len, HMACMD5Context *ctx);
void hmac_md5_final(uchar *digest, HMACMD5Context *ctx);

void arcfour(uint8_t *data, int len, const uchar *key, int key_len);

void cred_hash2(uint8_t *out, const uint8_t *in, const uint8_t *key, int forw);
void des_crypt56(uint8_t *out, const uint8_t *in, const uint8_t *key, int forw);

