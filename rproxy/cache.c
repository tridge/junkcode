#include "rproxy.h"

/* cache handling code. */

/* we use the complete request line hashed then base64 encoded to give the
   name of the cache file */
static char *cache_name(char *request, int no_query)
{
	unsigned char buf[16];
	static char cachename[1024];
	char b64[23];
	char *req = xstrdup(request);
	char *p;

	if (no_query) {
		p = strchr(req,'?');
		if (p) *p = 0;
	}

	mdfour(buf, (unsigned char *)req, strlen(req));
	base64_encode(buf, 16, b64);

	string_sub(b64, "/", "_");

	slprintf(cachename, sizeof(cachename), "%s/%s", CACHE_DIR, b64);

	free(req);

	return cachename;
}


FILE *cache_open(char *request, char *mode)
{
	FILE *f;
	char *name;
	name = cache_name(request, 0);
	f = fopen(name, mode);
	if (!f) {
		name = cache_name(request, 1);
		f = fopen(name, mode);
	}
	if (f) {
		struct stat st;
		stat(name, &st);
		logmsg("opened cache file %s of size %d\n", name, st.st_size);
	}
	return f;
}

static char cache_tname[1024];

FILE *cache_open_tmp(char *request, char *mode)
{
	slprintf(cache_tname, sizeof(cache_tname), "%s.tmp%u", 
		 cache_name(request, 0), (unsigned)getpid());
	return fopen(cache_tname, mode);
}

void cache_tmp_rename(char *request)
{
	char *cname = cache_name(request, 0);

	unlink(cname);

	if (rename(cache_tname, cname) != 0) {
		perror(cache_tname);
	}
}

void cache_tmp_delete(char *request)
{
	unlink(cache_tname);
}
