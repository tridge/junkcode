/*
  ADS netlogon crypt-challenge

  have fun!

  if you solve this, please email tridge@samba.org
 */

#include "includes.h"

/*
This is a netlogon.log snippet from a win2003 server when the ADS style netlogon
is negotiated. It shows all of the values that the w2k3 netlogon server is using
in the credential calculation.

06/01 12:41:45 [SESSION] VNET3: NetrServerAuthenticate entered: torturetest on account torturetest$ (Negot: 600fffff)
06/01 12:41:45 [SESSION] NetrServerAuthenticate: Password for account torturetest$ = 5625b790 79ee9733 1c3bcaa0 50c497ea   ê.%V3óÓy†.;.Íó.P
06/01 12:41:45 [SESSION] NetrServerAuthenticate: ClientChallenge 1 = 25315f5a 5187e90e   Z_1%.ÈáQ
06/01 12:41:45 [SESSION] NetrServerAuthenticate: ServerChallenge 1 = ef420375 419b0633   u.B.3.õA
06/01 12:41:45 [SESSION] NetrServerAuthenticate: SessionKey 0 = 522e1185 40807109 75429e9d eeeec995   Ö..R	qÄ@ù.Buï.ÓÓ
06/01 12:41:45 [SESSION] NetrServerAuthenticate: ClientCredential 0 GOT  = b4d76308 2067f19e   .c...Òg 
06/01 12:41:45 [SESSION] NetrServerAuthenticate: ClientCredential 0 MADE = 300a3413 3c741f66   .4.0f.t<
*/

/*
  here are the same values as above, as convenient C values. Note that
  this relies on running this on a x86 machine.
*/
static const uint32 chal1[2] = {0x25315f5a, 0x5187e90e};
static const uint32 chal2[2] = {0xef420375, 0x419b0633};
static const uint32 pass[4] = {0x5625b790, 0x79ee9733, 0x1c3bcaa0, 0x50c497ea};
static const uint32 skey[4] = {0x522e1185, 0x40807109, 0x75429e9d, 0xeeeec995};
static const uint32 cred[2] = {0x300a3413, 0x3c741f66};

int main(void)
{
	unsigned char zero[4];
	unsigned char s[16];
	unsigned char tmp[16];
	uint32 *x;
	HMACMD5Context ctx;
	struct MD5Context md5;
	unsigned char data[8];

	memset(zero, 0, sizeof(zero));

	MD5Init(&md5);
	MD5Update(&md5, zero, sizeof(zero));
	MD5Update(&md5, (uchar*)chal1, sizeof(chal1));
	MD5Update(&md5, (uchar*)chal2, sizeof(chal2));
	MD5Final(tmp,&md5);
	hmac_md5_init_rfc2104((uchar *)pass, 16, &ctx);
	hmac_md5_update(tmp, sizeof(tmp), &ctx);
	hmac_md5_final(s, &ctx);

	x = (uint32 *)s;
	printf("s: %08x %08x %08x %08x\n", x[0], x[1], x[2], x[3]);

	if (memcmp(s, skey, 16) == 0) {
		printf("Session key right!\n");
	} else {
		printf("session key wrong: try again\n");
		exit(1);
	}

	/* we've got the session key right. Now we need to somehow
	   combine the session key with the client challenge to
	   produce the first credential. This is how the server proves
	   that it has calculated the session key correctly.

	   so, we need to find an algorithm that takes a 16 byte key
	   (the skey above) and a 8 byte value (chal1 above) and
	   generates the correct 8 byte value (the cred value above).

	   It is possible that the algorithm mixes in the server
	   challenge (chal2), the machine password (pass) or even the
	   machine or account name (see log snippet above for
	   those). I don't think this is particularly likely though,
	   as really that is the job of the session key.

	   The likely crypto algorithm components are HMAC(), MD5(),
	   MD4(), ARCFOUR() and various concatenation or truncation
	   operations or even the addition of well-known constants.

	   Please also see draft-brezak-win2k-krb-rc4-hmac-*.txt in
	   pub/samba/specs/ on samba.org for information about a very
	   similar problem and how it is dealt with in MS Kerberos.
	 */

	memcpy(data, chal2, 8);

	smbhash(tmp, chal1, skey, 1);
	smbhash(data, tmp, ((char *)skey)+7, 1);
	
	x = (uint32 *)data;
	printf("c: %08x %08x\n", x[0], x[1]);

	if (memcmp(data, cred, 8) == 0) {
		printf("Credential right!\n");
	} else {
		printf("credential wrong: try again\n");
		exit(1);
	}

	return 0;
}
