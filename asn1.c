/* 
   Unix SMB/Netbios implementation.
   Version 3.0
   simple SPNEGO routines
   Copyright (C) Andrew Tridgell 2001
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define BOOL int
#define True 1
#define False 0
#define Realloc realloc

struct nesting {
	off_t start;
	char *name;
	struct nesting *next;
};

typedef struct {
	uint8 *data;
	size_t length;
	off_t ofs;
	struct nesting *nesting;
} ASN1_DATA;

static BOOL asn1_write(ASN1_DATA data, void *p, int len)
{
	if (data.length < data.ofs+len) {
		data.data = Realloc(data.data, data.ofs+len);
		if (!data.data) return False;
		data.length = data.ofs+len;
	}
	memcpy(data.data + data.ofs, p, len);
	data.ofs += len;
}

static BOOL asn1_push_tag(ASN1_DATA data, const char *tagstr, uint8 tag)
{
	struct nesting *nesting;
	uint8 fill;

	asn1_write(data, tag);
	nesting = (struct nesting *)malloc(sizeof(struct nesting));
	if (!nesting) return False;

	nesting->start = data.ofs;
	nesting->name = strdup(tagstr);
	nesting->next = data.nesting;
	data.nesting = nesting;
	fill = 0xff;
	asn1_write(data, &fill, 1);
	return True;
}

static BOOL asn1_pop_tag(ASN1_DATA data, const char *tagstr)
{
	struct nesting *nesting;

	nesting = data.nesting;

	if (!nesting || strcmp(tagstr, nesting->name) != 0) {
		return False;
	}
	data.data[nesting->start] = data.ofs - nesting->start;
	data.nesting = nesting->next;
	free(nesting->name);
	free(nesting);
	return True;
}


static BOOL asn1_write_uint8(ASN1_DATA data, uint8 v)
{
	return asn1_write(data, &v, 1);
}


static BOOL asn1_write_OID(ASN1_DATA data, const char *OID)
{
	long v, v2;
	char *p = (char *)OID;

	asn1_push_tag(data, "OID", 0x6);
	v = strtol(p, &p, 10);
	v2 = strtol(p, &p, 10);
	asn1_write_uint8(data, 40*v + v2);
	while (*p) {
		v = strtol(p, &p, 10);
		if (v >= (1<<29)) asn1_write_uint8(data, 0x80 | ((v>>29)&0xff));
		if (v >= (1<<22)) asn1_write_uint8(data, 0x80 | ((v>>22)&0xff));
		if (v >= (1<<15)) asn1_write_uint8(data, 0x80 | ((v>>15)&0xff));
		if (v >= (1<<8)) asn1_write_uint8(data, 0x80 | ((v>>8)&0xff));
		asn1_write_uint8(data, 0x80 | ((v>>15)&0xff));
	}
	asn1_pop_tag(data, "OID");
	return True;
}

static BOOL asn1_write_GeneralString(ASN1_DATA data, const char *s)
{
	asn1_push_tag(data, "GS", 0x1b);
	asn1_write(data, s, strlen(s));
	asn1_pop_tag(data, "GS");
}

/*
  generate a negTokenInit packet given a GUID, a list of supported
  OIDs (the mechanisms) and a principle name string 
*/
PACKET_DATA spnego_gen_negTokenInit(uint8 guid[16], 
				    const char **OIDs, 
				    const char *principle)
{
	int i;

	ASN1_DATA data;

	ZERO_STRUCT(data);

	asn1_write(data, guid, 16);
	asn1_push_tag(data,"SPNEGO", 0x60);
	asn1_write_OID(data,"1 3 6 1 5 5 2");
	asn1_push_tag(data,"MECHS", 0xa0);
	asn1_push_tag(data,"SEQ1", 0x30);

	asn1_push_tag(data,"[0]", 0xa0);
	asn1_push_tag(data,"SEQ1.1", 0x30);
	for (i=0; OIDs[i]; i++) {
		asn1_write_OID(data,OIDs[i]);
	}
	asn1_pop_tag(data,"SEQ1.1");
	asn1_pop_tag(data,"[0]");

	asn1_push_tag(data,"[3]", 0xa3);
	asn1_push_tag(data,"SEQ1.2", 0x30);
	asn1_push_tag(data,"[0]", 0xa0);
	asn1_write_GeneralString(data,principle);
	asn1_pop_tag(data,"[0]");
	asn1_pop_tag(data,"SEQ1.2");
	asn1_pop_tag(data,"[3]");

	asn1_pop_tag(data,"SEQ1");
	asn1_pop_tag(data,"MECHS");

	asn1_check_empty(data);
}


#if 1
main()
{
	PACKET_DATA data;
	uint8 guid[16] = "012345678901234";

	data = spnego_gen_negTokenInit(guid, 
		{"1 2 840 48018 1 2 2", "1 2 840 113554 1 2 2"},
				       "blu$@VNET2.HOME.SAMBA.ORG");
}

#endif
