#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/socket.h>
#include <getopt.h>
#include <krb5.h>
#include <lber.h>
#include <ldap.h>
#include <sasl.h>
#include "ads.h"

#define VERSION "3.0 prealpha"

typedef unsigned BOOL;
typedef unsigned char uint8;
#define True 1
#define False 0

typedef struct {
	uint8 *data;
	size_t length;
} DATA_BLOB;

#define ZERO_STRUCTP(x) do { if ((x) != NULL) memset((char *)(x), 0, sizeof(*(x))); } while(0)

#define ZERO_STRUCT(x) memset((char *)&(x), 0, sizeof(x))

#define SAFE_FREE(x) do { if ((x) != NULL) {free(x); x=NULL;} } while(0)

#define Realloc realloc
#define xmalloc malloc

#define DEBUG(level, p) printf p

typedef char pstring[1024];
typedef char fstring[128];

#define pstrcat(d,s) strcat((d),(s))

static void *memdup(void *p, size_t size)
{
	void *p2;
	if (size == 0) return NULL;
	p2 = malloc(size);
	if (!p2) return NULL;
	memcpy(p2, p, size);
	return p2;
}

#include "asn1.h"

#define UF_DONT_EXPIRE_PASSWD           0x10000
#define UF_MNS_LOGON_ACCOUNT            0x20000
#define UF_SMARTCARD_REQUIRED           0x40000
#define UF_TRUSTED_FOR_DELEGATION       0x80000
#define UF_NOT_DELEGATED               0x100000
#define UF_USE_DES_KEY_ONLY            0x200000
#define UF_DONT_REQUIRE_PREAUTH        0x400000

#define UF_TEMP_DUPLICATE_ACCOUNT       0x0100
#define UF_NORMAL_ACCOUNT               0x0200
#define UF_INTERDOMAIN_TRUST_ACCOUNT    0x0800
#define UF_WORKSTATION_TRUST_ACCOUNT    0x1000
#define UF_SERVER_TRUST_ACCOUNT         0x2000


int ldap_add_machine_account(const char *ldap_host, 
			     const char *hostname, const char *realm);

int krb5_set_principal_password(char *principal, 
				const char *kdc_host, 
				const char *hostname, 
				const char *realm);

krb5_error_code 
krb5_set_password(krb5_context context,
		  krb5_ccache ccache,
		  char *newpw,
		  krb5_principal targprinc,
		  int *result_code,
		  krb5_data *result_code_string,
		  krb5_data *result_string,
		  const char *kdc_host,
		  const char *hostname,
		  const char *realm);


krb5_error_code krb5_locate_kpasswd(krb5_context context,
				    const krb5_data *realm,
				    struct sockaddr **addr_pp,
				    int *naddrs);

void asn1_free(ASN1_DATA *data);
BOOL asn1_write(ASN1_DATA *data, const void *p, int len);
BOOL asn1_write_uint8(ASN1_DATA *data, uint8 v);
BOOL asn1_push_tag(ASN1_DATA *data, uint8 tag);
BOOL asn1_pop_tag(ASN1_DATA *data);
BOOL asn1_write_OID(ASN1_DATA *data, const char *OID);
BOOL asn1_write_OctetString(ASN1_DATA *data, const void *p, size_t length);
BOOL asn1_write_GeneralString(ASN1_DATA *data, const char *s);
BOOL asn1_write_BOOLEAN(ASN1_DATA *data, BOOL v);
BOOL asn1_check_BOOLEAN(ASN1_DATA *data, BOOL v);
BOOL asn1_load(ASN1_DATA *data, DATA_BLOB blob);
BOOL asn1_read(ASN1_DATA *data, void *p, int len);
BOOL asn1_read_uint8(ASN1_DATA *data, uint8 *v);
BOOL asn1_start_tag(ASN1_DATA *data, uint8 tag);
BOOL asn1_end_tag(ASN1_DATA *data);
int asn1_tag_remaining(ASN1_DATA *data);
BOOL asn1_read_OID(ASN1_DATA *data, char **OID);
BOOL asn1_check_OID(ASN1_DATA *data, char *OID);
BOOL asn1_read_GeneralString(ASN1_DATA *data, char **s);
BOOL asn1_read_OctetString(ASN1_DATA *data, DATA_BLOB *blob);
BOOL asn1_check_enumerated(ASN1_DATA *data, int v);
BOOL asn1_write_enumerated(ASN1_DATA *data, uint8 v);
BOOL asn1_write_Integer(ASN1_DATA *data, int i);
