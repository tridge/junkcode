/*--

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Copyright (C) 1999  Microsoft Corporation.  All rights reserved.

Module Name:

    ksetpw.c

Abstract:

    Set a user's password using the
    Kerberos Change Password Protocol (I-D) variant for Windows 2000

--*/
/*
 * lib/krb5/os/changepw.c
 *
 * Copyright 1990 by the Massachusetts Institute of Technology.
 * All Rights Reserved.
 *
 * Export of this software from the United States of America may
 *   require a specific license from the United States Government.
 *   It is the responsibility of any person or organization contemplating
 *   export to obtain such a license before exporting.
 * 
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of M.I.T. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 * 
 */

#include "includes.h"

#define krb5_is_krb_error(dat)\
	((dat) && (dat)->length && ((dat)->data[0] == 0x7e ||\
				    (dat)->data[0] == 0x5e))

void file_save(const char *fname, void *packet, size_t length)
{
	int fd;
	fd = open(fname, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	if (fd == -1) return;
	write(fd, packet, length);
	close(fd);
}

/*******************************************************************
 construct a data blob, must be freed with data_blob_free()
*******************************************************************/
DATA_BLOB data_blob(void *p, size_t length)
{
	DATA_BLOB ret;

	if (!p || !length) {
		ZERO_STRUCT(ret);
		return ret;
	}

	ret.data = memdup(p, length);
	ret.length = length;
	return ret;
}


#define DEFAULT_KPASSWD_PORT	464


#define KRB5_KPASSWD_VERS_CHANGEPW	1
#define KRB5_KPASSWD_VERS_SETPW		0xff80
#define KRB5_KPASSWD_ACCESSDENIED	5
#define KRB5_KPASSWD_BAD_VERSION	6
#define KRB5_KPASSWD_INITIAL_FLAG_NEEDED 7

/* This implements the Kerb password change protocol as specifed in
 * kerb-chg-password-02.txt
 */
static DATA_BLOB encode_krb5_setpw(const char *hostname, 
				   const char *realm, const char *password)
{
	ASN1_DATA req;
	DATA_BLOB ret;

	memset(&req, 0, sizeof(req));
	
	asn1_push_tag(&req, ASN1_SEQUENCE(0));
	asn1_push_tag(&req, ASN1_CONTEXT(0));
	asn1_write_OctetString(&req, password, strlen(password));
	asn1_pop_tag(&req);

	asn1_push_tag(&req, ASN1_CONTEXT(1));
	asn1_push_tag(&req, ASN1_SEQUENCE(0));

	asn1_push_tag(&req, ASN1_CONTEXT(0));
	asn1_write_Integer(&req, 1);
	asn1_pop_tag(&req);

	asn1_push_tag(&req, ASN1_CONTEXT(1));
	asn1_push_tag(&req, ASN1_SEQUENCE(0));
	asn1_write_GeneralString(&req, "HOST");
	asn1_write_GeneralString(&req, hostname);
	asn1_pop_tag(&req);
	asn1_pop_tag(&req);
	asn1_pop_tag(&req);
	asn1_pop_tag(&req);

	asn1_push_tag(&req, ASN1_CONTEXT(2));
	asn1_write_GeneralString(&req, realm);
	asn1_pop_tag(&req);
	asn1_pop_tag(&req);

	ret = data_blob(req.data, req.length);
	asn1_free(&req);

	return ret;
}	

static krb5_error_code krb5_mk_setpw_req(krb5_context context,
					 krb5_auth_context auth_context,
					 krb5_data *ap_req,
					 const char *hostname,
					 const char *realm,
					 const char *passwd,
					 krb5_data *packet)
{
	krb5_error_code ret;
	krb5_data cipherpw;
	krb5_data encoded_setpw;
	krb5_replay_data replay;
	char *ptr;
	DATA_BLOB setpw;

	if ((ret = krb5_auth_con_setflags(context, auth_context,
					  KRB5_AUTH_CONTEXT_DO_SEQUENCE)))
		return(ret);

	setpw = encode_krb5_setpw(hostname, realm, passwd);

	encoded_setpw.data = setpw.data;
	encoded_setpw.length = setpw.length;

	if ((ret = krb5_mk_priv(context, auth_context,
				&encoded_setpw, &cipherpw, &replay)))
		return(ret);
	
	packet->length = 6 + ap_req->length + cipherpw.length;
	packet->data = (char *) malloc(packet->length);
	ptr = packet->data;
	
	/* Length */
	*ptr++ = (packet->length>>8) & 0xff;
	*ptr++ = packet->length & 0xff;
	/* version */

	*ptr++ = (char)0xff;
	*ptr++ = (char)0x80;

	/* ap_req length, big-endian */

	*ptr++ = (ap_req->length>>8) & 0xff;
	*ptr++ = ap_req->length & 0xff;
	
	/* ap-req data */
	
	memcpy(ptr, ap_req->data, ap_req->length);
	ptr += ap_req->length;
	
	/* krb-priv of password */
	
	memcpy(ptr, cipherpw.data, cipherpw.length);
	
	return(0);
}

static krb5_error_code krb5_rd_setpw_rep(krb5_context context, 
					 krb5_auth_context auth_context,
					 krb5_data *packet,
					 int *result_code,
					 krb5_data *result_data)
{
	char *ptr;
	int plen, vno;
	krb5_data ap_rep;
	krb5_ap_rep_enc_part *ap_rep_enc;
	krb5_error_code ret;
	krb5_data cipherresult;
	krb5_data clearresult;
	krb5_error *krberror;
	krb5_replay_data replay;
	
	if (packet->length < 4)
		/* either this, or the server is printing bad messages,
		   or the caller passed in garbage */
		return(KRB5KRB_AP_ERR_MODIFIED);
	
	ptr = packet->data;
	
	if (krb5_is_krb_error(packet)) {
		file_save("error.dat", packet->data, packet->length);
		return -1;
	}
	
	/* verify length */
	
	plen = (*ptr++ & 0xff);
	plen = (plen<<8) | (*ptr++ & 0xff);
	
	if (plen != packet->length)
		return(KRB5KRB_AP_ERR_MODIFIED);
	
	vno = (*ptr++ & 0xff);
	vno = (vno<<8) | (*ptr++ & 0xff);
	
	if (vno != KRB5_KPASSWD_VERS_SETPW &&
	    vno != KRB5_KPASSWD_VERS_CHANGEPW)
		return(KRB5KDC_ERR_BAD_PVNO);
	
	/* read, check ap-rep length */
	
	ap_rep.length = (*ptr++ & 0xff);
	ap_rep.length = (ap_rep.length<<8) | (*ptr++ & 0xff);
	
	if (ptr + ap_rep.length >= packet->data + packet->length)
		return(KRB5KRB_AP_ERR_MODIFIED);
	
	if (ap_rep.length) {
		/* verify ap_rep */
		ap_rep.data = ptr;
		ptr += ap_rep.length;
		
		if ((ret = krb5_rd_rep(context, auth_context, &ap_rep, &ap_rep_enc)))
			return(ret);
		
		krb5_free_ap_rep_enc_part(context, ap_rep_enc);
		
		/* extract and decrypt the result */
		
		cipherresult.data = ptr;
		cipherresult.length = (packet->data + packet->length) - ptr;
		
		ret = krb5_rd_priv(context, auth_context, &cipherresult, &clearresult,
				   &replay);
		
		if (ret)
			return(ret);
	} else {
		cipherresult.data = ptr;
		cipherresult.length = (packet->data + packet->length) - ptr;
		
		if ((ret = krb5_rd_error(context, &cipherresult, &krberror)))
			return(ret);
		
		clearresult = krberror->e_data;
	}

	if (clearresult.length < 2) {
		ret = KRB5KRB_AP_ERR_MODIFIED;
		goto cleanup;
	}
	
	ptr = clearresult.data;
	
	*result_code = (*ptr++ & 0xff);
	*result_code = (*result_code<<8) | (*ptr++ & 0xff);
	
	if ((*result_code < KRB5_KPASSWD_SUCCESS) ||
	    (*result_code > KRB5_KPASSWD_ACCESSDENIED)) {
		ret = KRB5KRB_AP_ERR_MODIFIED;
		goto cleanup;
	}
	
	/* all success replies should be authenticated/encrypted */
	
	if ((ap_rep.length == 0) && (*result_code == KRB5_KPASSWD_SUCCESS)) {
		ret = KRB5KRB_AP_ERR_MODIFIED;
		goto cleanup;
	}
	
	result_data->length = (clearresult.data + clearresult.length) - ptr;
	
	if (result_data->length) {
		result_data->data = (char *) malloc(result_data->length);
		memcpy(result_data->data, ptr, result_data->length);
	} else {
		result_data->data = NULL;
	}
	
	ret = 0;
	
cleanup:
	if (ap_rep.length) {
		free(clearresult.data);
	} else {
		krb5_free_error(context, krberror);
	}
	
	return(ret);
}

static int open_socket(const char *host, int port)
{
	int type = SOCK_DGRAM;
	struct sockaddr_in sock_out;
	int res;
	struct hostent *hp;  

	res = socket(PF_INET, type, 0);
	if (res == -1) {
		return -1;
	}

	hp = gethostbyname(host);
	if (!hp) {
		fprintf(stderr,"unknown host: %s\n", host);
		return -1;
	}

	memcpy(&sock_out.sin_addr, hp->h_addr, hp->h_length);
	sock_out.sin_port = htons(port);
	sock_out.sin_family = PF_INET;

	if (connect(res,(struct sockaddr *)&sock_out,sizeof(sock_out))) {
		close(res);
		fprintf(stderr,"failed to connect to %s - %s\n", 
			host, strerror(errno));
		return -1;
	}

	return res;
}



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
		  const char *realm)
{
	krb5_auth_context auth_context;
	krb5_data ap_req, chpw_req, chpw_rep;
	krb5_address local_kaddr, remote_kaddr;
	char *host;
	krb5_error_code code;
	krb5_creds creds, *credsp;
	int addr_len;
	struct sockaddr remote_addr, local_addr;
	int cc, local_result_code;
	int s1;
	int hostlen;
	
	auth_context = NULL;
	
	memset(&creds, 0, sizeof(creds));
	
	if ((code = krb5_parse_name(context, "kadmin/changepw", &creds.server)))
		return(code);
	
	/* Make target service same as target princ */
	krb5_princ_set_realm(context, creds.server,
			     krb5_princ_realm(context, targprinc));
	
	if ((code = krb5_cc_get_principal(context, ccache, &creds.client))) {
		krb5_free_principal(context, creds.server);
		return(code);
	}
	
	if ((code = krb5_get_credentials(context, 0, ccache, &creds, &credsp))) {
		krb5_free_principal(context, creds.server);
		krb5_free_principal(context, creds.client);
		return(code);
	}
	
	if ((code = krb5_mk_req_extended(context, &auth_context, AP_OPTS_USE_SUBKEY,
					 NULL, credsp, &ap_req)))
		return(code);
	
	if ((host = malloc(krb5_princ_realm(context, credsp->client)->length + 1))
	    == NULL) 
		return ENOMEM;
	
	hostlen = krb5_princ_realm(context, targprinc)->length;
	strncpy(host, krb5_princ_realm(context, targprinc)->data, hostlen);
	
	host[hostlen] = '\0';
	
	s1 = open_socket(kdc_host, DEFAULT_KPASSWD_PORT);
	
	addr_len = sizeof(remote_addr);
	getpeername(s1, &remote_addr, &addr_len);
	addr_len = sizeof(local_addr);
	getsockname(s1, &local_addr, &addr_len);
	
	remote_kaddr.addrtype = ADDRTYPE_INET;
	remote_kaddr.length =
	    sizeof(((struct sockaddr_in *) &remote_addr)->sin_addr);
	remote_kaddr.contents = 
	    (char *) &(((struct sockaddr_in *) &remote_addr)->sin_addr);

	local_kaddr.addrtype = ADDRTYPE_INET;
	local_kaddr.length =
	    sizeof(((struct sockaddr_in *) &local_addr)->sin_addr);
	local_kaddr.contents = 
	    (char *) &(((struct sockaddr_in *) &local_addr)->sin_addr);

	if ((code = krb5_auth_con_setaddrs(context, auth_context, &local_kaddr,
					  NULL))) {
	    close(s1);
	    return(code);
	}

	if ((code = krb5_mk_setpw_req(context, auth_context, &ap_req,
				      hostname, realm, newpw, &chpw_req))) {
	    close(s1);
	    return(code);
	}

	if ((cc = write(s1, chpw_req.data, chpw_req.length)) != 
	    chpw_req.length) {
	    close(s1);
	    return((cc < 0)?errno:ECONNABORTED);
	}

	free(chpw_req.data);

	chpw_rep.length = 1500;
	chpw_rep.data = (char *) malloc(chpw_rep.length);

	if ((cc = read(s1, chpw_rep.data, chpw_rep.length)) < 0) {
	    close(s1);
	    return(errno);
	}

	close(s1);

	chpw_rep.length = cc;

	if ((code = krb5_auth_con_setaddrs(context, auth_context, NULL,
					  &remote_kaddr))) {
	    close(s1);
	    return(code);
	}

	code = krb5_rd_setpw_rep(context, auth_context, &chpw_rep,
				&local_result_code, result_string);

	free(chpw_rep.data);

	if (code)
	    return(code);

	if (result_code)
	    *result_code = local_result_code;

	return(0);
}


