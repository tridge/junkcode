/*--

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 1999  Microsoft Corporation.  All rights reserved.

Module Name:

    netjoin.c

Abstract:

    Sample program that demonstrates how to create a computer account
    using LDAP.

--*/

#include "includes.h"

/* a lame random number generator - used /dev/urandom if possible */
static unsigned one_random(void)
{
	int fd;
	static int initialised;
	unsigned ret;

	if (!initialised) {
		initialised = 1;
		fd = open("/dev/urandom", O_RDONLY);
		srandom(time(NULL) ^ getpid());
	}

	if (fd == -1) {
		return random();
	}

	read(fd, &ret, sizeof(ret));
	return ret;
}

/*
 * Generate a simple random password of 15 chars - not a cryptographic one
 */
static int generate_random_password(krb5_data *pwd)
{
    int i;

    pwd->length = 15;

    if (!(pwd->data = malloc(pwd->length+1)))
	return ENOMEM;
    pwd->magic = KV5M_PWD_DATA;

    memset(pwd->data, 0, pwd->length+1);
    
    for (i=0; i<pwd->length; ) {
	    char c = one_random() & 0x7f;
	    if (!isalnum(c) && !ispunct(c)) continue;
	    pwd->data[i++] = c;
    }

    return 0;
}

int krb5_set_principal_password(char *principal, 
				const char *kdc_host, 
				const char *hostname, 
				const char *realm)
{
	krb5_context context;
	krb5_data salt;
	krb5_encrypt_block eblock;
	krb5_keyblock * key;
	krb5_data password;
	int res_code;
	krb5_data res_code_str, res_string;
	krb5_keytab kt;
	krb5_keytab_entry entry;
	krb5_kt_cursor cursor;
	krb5_principal host_princ;
	krb5_error_code retval;
	krb5_ccache ccache;
	int kvno = 1, i;
	char ktname[MAXPATHLEN+sizeof("WRFILE:")+1];
	extern krb5_kt_ops krb5_ktf_writable_ops;
	
	retval = krb5_init_context(&context);
	if (retval) {
		return retval;
	}
	
	if ((retval = krb5_cc_default(context, &ccache))) {
		com_err("keytab", retval, "opening default ccache");
		exit(1);
	}
	
	retval = krb5_parse_name(context, principal, &host_princ);
	if (retval)
		return retval;
	
#define NUMTRIES  10

	printf("Setting computer password...\n");

	for (i = 0; i < NUMTRIES; i++) {
		generate_random_password(&password);

		retval = krb5_set_password(context, ccache, password.data, host_princ,
					   &res_code, &res_code_str, &res_string,
					   kdc_host, hostname, realm);

		if (retval) {
			com_err("keytab", retval, "setting computer password");
			exit(1);
		}
		if (res_code == KRB5_KPASSWD_SUCCESS)
			break;
		else if (res_code != KRB5_KPASSWD_MALFORMED) {
			printf("Set password for computer account failed: (%d) %.*s%s%.*s\n",
			       res_code, res_code_str.length, res_code_str.data,
			       res_string.length?": ":"",
			       res_string.length, res_string.data);
			exit(2);
		}
	}
	if (i == NUMTRIES)	/* Can't generate a good random password */
		return KRB5KRB_AP_ERR_NOKEY;

	retval = krb5_principal2salt(context, host_princ, &salt);
	if (retval)
		return retval;
    
	if (!(key = (krb5_keyblock *)malloc(sizeof(*key))))
		return ENOMEM;
	
	krb5_use_enctype(context, &eblock, ENCTYPE_DES_CBC_MD5);
	
	retval = krb5_string_to_key(context, &eblock, key, &password, &salt);
	if (retval)
		return retval;
	
	retval = krb5_kt_register(context, &krb5_ktf_writable_ops);
	if (retval) {
		com_err("keytab", retval,
			"while registering writable key table functions");
		exit(1);
	}
	
	krb5_kt_default_name(context, &ktname[2], sizeof(ktname)-2);
	ktname[0] = 'W';
	ktname[1] = 'R';
	
	printf("default keytab name - \"%s\"\n", ktname);
	
	retval = krb5_kt_resolve(context, ktname, &kt);
	if (retval) {
		com_err("keytab", retval, "resolving keytab");
		return retval;
	}
	
	retval = krb5_kt_start_seq_get(context, kt, &cursor);
	if (retval == 0) {
		for (;;) {
			retval = krb5_kt_next_entry(context, kt, &entry, &cursor);
			if (retval)
				break;
			if (krb5_principal_compare(context, entry.principal, host_princ)) {
				printf("Found existing entry in keytab\n");
				if (entry.vno > kvno)
					kvno = entry.vno;
			}
		}
		if (retval == KRB5_KT_END)
			retval = 0;
		krb5_kt_end_seq_get(context, kt, &cursor);
		kvno++;
	}
	
	entry.magic = KV5M_KEYTAB;
	entry.principal = host_princ;
	entry.vno = kvno;
	entry.key = *key;
	printf("Saving key version %d in %s\n", kvno, ktname);
	retval = krb5_kt_add_entry(context, kt, &entry);
	if (retval) 
		com_err("keytab", retval, "writing keytab");
	
	krb5_kt_close(context, kt);
	
	if (retval == 0) {
		printf("Successfully set machine account password\n");
	}
	
	return retval;
}
