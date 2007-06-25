/* list users for a particular nss module
 */

#include <stdio.h>
#include <nss.h>
#include <dlfcn.h>
#include <pwd.h>
#include <grp.h>

typedef enum nss_status NSS_STATUS;

static char *nss_name;
static int nss_errno;
static NSS_STATUS last_error;
static int total_errors;

static void *find_fn(const char *name)
{
	char *so_path, *s;
	static void *h;
	void *res;

	asprintf(&s, "_nss_%s_%s", nss_name, name);
	asprintf(&so_path, "/lib/libnss_%s.so.2", nss_name);

	if (!h) {
		h = dlopen(so_path, RTLD_LAZY);
	}
	if (!h) {
		printf("Can't open shared library %s\n", so_path);
		exit(1);
	}
	res = dlsym(h, s);
	if (!res) {
		printf("Can't find function %s\n", s);
		return NULL;
	}
	free(so_path);
	free(s);
	return res;
}

static void report_nss_error(const char *who, NSS_STATUS status)
{
	last_error = status;
	total_errors++;
	printf("ERROR %s: NSS_STATUS=%d  %d (nss_errno=%d)\n", 
	       who, status, NSS_STATUS_SUCCESS, nss_errno);
}

static struct passwd *nss_getpwent(void)
{
	NSS_STATUS (*_nss_getpwent_r)(struct passwd *, char *, 
				      size_t , int *) = find_fn("getpwent_r");
	static struct passwd pwd;
	static char buf[1000];
	NSS_STATUS status;

	status = _nss_getpwent_r(&pwd, buf, sizeof(buf), &nss_errno);
	if (status == NSS_STATUS_NOTFOUND) {
		return NULL;
	}
	if (status != NSS_STATUS_SUCCESS) {
		report_nss_error("getpwent", status);
		return NULL;
	}
	return &pwd;
}

static void nss_setpwent(void)
{
	NSS_STATUS (*_nss_setpwent)(void) = find_fn("setpwent");
	NSS_STATUS status;
	status = _nss_setpwent();
	if (status != NSS_STATUS_SUCCESS) {
		report_nss_error("setpwent", status);
	}
}

static void nss_endpwent(void)
{
	NSS_STATUS (*_nss_endpwent)(void) = find_fn("endpwent");
	NSS_STATUS status;
	status = _nss_endpwent();
	if (status != NSS_STATUS_SUCCESS) {
		report_nss_error("endpwent", status);
	}
}

static void list_users(void)
{
	struct passwd *pwd;

	nss_setpwent();
	/* loop over all users */
	while ((pwd = nss_getpwent())) {
		printf("%s\n", pwd->pw_name);
	}
	nss_endpwent();
}

static void usage(void)
{
	printf("nsslist <nsstype>\n");
}

int main(int argc, char *argv[])
{	
	if (argc < 2) {
		usage();
		exit(1);
	}

	nss_name = argv[1];

	list_users();

	return 0;
}
