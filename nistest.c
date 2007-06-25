/************************************************
nss_funcs.c
-------------------------------------------------
based on nsstest.c by a. tridgell
expanded by j. trostel
-------------------------------------------------
get user and group info for different nss modules
************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <nss.h>
#include <dlfcn.h>
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>

typedef enum nss_status NSS_STATUS;
int nss_errno;

void *find_fn(const char *nss_name, const char *funct_name)
{
	char *so_path, *s;
	void *h;
	void *res;

	// create the library name
	asprintf(&so_path, "libnss_%s.so.2", nss_name);
	h = dlopen(so_path, RTLD_LAZY);

	if (!h){
		free(so_path);
		exit(1);
	}

	// create the function name
	asprintf(&s, "_nss_%s_%s", nss_name, funct_name);

	// and find it in the library
	res = dlsym(h, s);

	if (!res) {
		free(so_path);
		free(s);
		return NULL;
	}

	// free the strings from asprintf
	free(so_path);
	free(s);
	return res;
}

struct passwd *nss_getpwent(const char *nss_name)
{
	NSS_STATUS (*_nss_getpwent_r)(struct passwd *, char *, 
				      size_t , int *) = find_fn(nss_name, "getpwent_r");
	static struct passwd pwd;
	static char buf[1000];
	NSS_STATUS status;

	status = _nss_getpwent_r(&pwd, buf, sizeof(buf), &nss_errno);

	if (status != NSS_STATUS_SUCCESS)
		return NULL;

	return &pwd;
}

struct passwd *nss_getpwnam(const char *nss_name, const char *name)
{
	NSS_STATUS (*_nss_getpwnam_r)(const char *, struct passwd *, char *, 
				      size_t , int *) = find_fn(nss_name, "getpwnam_r");
	static struct passwd pwd;
	static char buf[1000];
	NSS_STATUS status;

	status = _nss_getpwnam_r(name, &pwd, buf, sizeof(buf), &nss_errno);

	if (status != NSS_STATUS_SUCCESS) 
		return NULL;

	return &pwd;
}

struct passwd *nss_getpwuid(const char *nss_name, uid_t uid)
{
	NSS_STATUS (*_nss_getpwuid_r)(uid_t, struct passwd *, char *, 
				      size_t , int *) = find_fn(nss_name, "getpwuid_r");
	static struct passwd pwd;
	static char buf[1000];
	NSS_STATUS status;

	status = _nss_getpwuid_r(uid, &pwd, buf, sizeof(buf), &nss_errno);

	if (status != NSS_STATUS_SUCCESS) 
		return NULL;

	return &pwd;
}

NSS_STATUS nss_setpwent(const char *nss_name)
{
	NSS_STATUS (*_nss_setpwent)(void) = find_fn(nss_name, "setpwent");
	return( _nss_setpwent() );
}

NSS_STATUS nss_endpwent(const char *nss_name)
{
	NSS_STATUS (*_nss_endpwent)(void) = find_fn(nss_name, "endpwent");
	return( _nss_endpwent() );
}

struct group *nss_getgrent(const char *nss_name)
{
	NSS_STATUS (*_nss_getgrent_r)(struct group *, char *, 
				      size_t , int *) = find_fn(nss_name, "getgrent_r");
	static struct group grp;
	int my_errno = 0;
	static char *buf = NULL;
	static int buflen = 1024;
	NSS_STATUS status;
	
	if(!buf) {
		if((buf = malloc(buflen)) == NULL)
		return NULL;
	}
	
again:
	status = _nss_getgrent_r(&grp, buf, buflen, &my_errno);
	if (status == NSS_STATUS_TRYAGAIN) {
		buflen *= 2;
		free(buf);
		if( (buf = malloc(buflen)) == NULL) {
			buflen = 1024;
			return NULL;
		}
		goto again;
	}

	if (status != NSS_STATUS_SUCCESS) {
		free(buf);
		buf = NULL;
		return NULL;
	}

	return &grp;
}

struct group *nss_getgrnam(const char *nss_name, const char *name)
{
	NSS_STATUS (*_nss_getgrnam_r)(const char *, struct group *, char *, 
				      size_t , int *) = find_fn(nss_name, "getgrnam_r");
	static struct group grp;
	static char *buf = NULL;
	static int buflen = 1024;
	NSS_STATUS status;

	if (!buf) {
		if( (buf = malloc(buflen)) == NULL)
			return NULL;
	}
	
again:
	status = _nss_getgrnam_r(name, &grp, buf, buflen, &nss_errno);
	if (status == NSS_STATUS_TRYAGAIN) {
		buflen *= 2;
		free(buf);
		if ( (buf = malloc(buflen)) == NULL) {
			buflen = 1024;
			return NULL;
		}
		goto again;
	}

	if (status != NSS_STATUS_SUCCESS) {
		free(buf);
		buf = NULL;
		return NULL;
	}

	return &grp;
}

struct group *nss_getgrgid(const char *nss_name, gid_t gid)
{
	NSS_STATUS (*_nss_getgrgid_r)(gid_t, struct group *, char *, 
				      size_t , int *) = find_fn(nss_name, "getgrgid_r");
	static struct group grp;
	static char *buf = NULL;
	static int buflen = 1024;
	NSS_STATUS status;
	
	if (!buf) {
		if( (buf = malloc(buflen)) == NULL)
			return NULL;
	}
	
again:
	status = _nss_getgrgid_r(gid, &grp, buf, buflen, &nss_errno);
	if (status ==  NSS_STATUS_TRYAGAIN) {
		buflen *= 2;
		free(buf);
		if( (buf = malloc(buflen)) == NULL) {
			buflen = 1024;
			return NULL;
		}
		goto again;
	}
	
	if (status != NSS_STATUS_SUCCESS) {
		free(buf);
		buf = NULL;
		return NULL;
	}

	return &grp;
}
	

NSS_STATUS nss_setgrent(const char *nss_name)
{
	NSS_STATUS (*_nss_setgrent)(void) = find_fn(nss_name, "setgrent");
	return( _nss_setgrent() );
}

NSS_STATUS nss_endgrent(const char *nss_name)
{
	NSS_STATUS (*_nss_endgrent)(void) = find_fn(nss_name, "endgrent");
	return( _nss_endgrent() );
}

/*
  this trick resets the internal glibc cached copy of the NIS domain name
  read the glibc nis code (in particular the __ypdomain[] static array) to
  understand whats going on
 */
static void nis_reset_domain(void)
{
	void (*fn_yp_get_default_domain)(char **);
	void *h = dlopen("libnss_nis.so.2", RTLD_LAZY);
	char *domain = NULL;

	fn_yp_get_default_domain = dlsym(h, "yp_get_default_domain");
	
	if (!fn_yp_get_default_domain) return;

	fn_yp_get_default_domain(&domain);
	if (domain) {
		domain[0] = 0;
	}
}

int main(void)
{
	while (1) {
		struct passwd *pwd;
		int i;

		sleep(1);

		nis_reset_domain();

		nss_setpwent("nis");
		i=0;
		while (nss_getpwent("nis")) i++;
		printf("i=%d\n", i);
		nss_endpwent("nis");
	}
	return 0;
}
