/*
  a simple UESS test harness for AIX

  Andrew Tridgell <tridge@au.ibm.com> 
  January 2004


  Usage:
      uess_test <MODULE_NAME> <nloops>

  for example "uess_test LDAP" will load the LDAP module using methods.cfg
  and will call directly into the supplied methods

  If nloops is not supplied then 1 is assumed. Note that the built-in
  memory leak checking can only be performed with nloops > 1


  You can force uess_test not to free the gr_passwd entry in struct group by setting the
  environment variable "DONT_FREE_GR_PASSWD". This is needed for the AIX 5.2 LDAP module,
  as it incorrectly returns a constant string in that field.


  I also strongly recommend you run this test program with the following malloc debug options set:

    MALLOCDEBUG=validate_ptrs,report_allocations,record_allocations
    MALLOCTYPE=debug


  Thanks to Julianne Haugh for assistance in understanding the UESS interface.

*/

#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <usersec.h>
#include <malloc.h>


static struct secmethod_table *methods;
static int err_count;

#define INVALID_STRING "INVALID"
#define INVALID_INT    123456

/*
  test method_attrlist 
 */
static void test_attrlist(void)
{
	attrlist_t **a;
	int i;

	if (!methods->method_attrlist) {
		printf("WARNING: module has no method_attrlist\n");
		return;
	}

	printf("\ntesting method_attrlist\n");

	a = methods->method_attrlist();
	if (!a) {
		printf("ERROR: attrlist failed - %s\n", strerror(errno));
		err_count++;
		return;
	}

	for (i=0;a[i];i++) {
		printf("\tattrib '%s' flags=0x%x type=%d\n", 
		       a[i]->al_name, a[i]->al_flags, a[i]->al_type);
		free(a[i]->al_name);
	}
	free(a);
}


/*
  test listing users
*/
static void test_userlist(void)
{
	int ret;
	attrval_t a;
	char *attribs = "users";
	char *s;

	if (!methods->method_getentry) {
		printf("WARNING: module has no method_getentry\n");
		return;
	}

	printf("\ntesting method_getentry(ALL, user)\n");

	ret = methods->method_getentry("ALL", "user", &attribs, &a, 1);
	if (ret != 0) {
		printf("ERROR: method_getentry failed ret=%d - %s\n", ret, strerror(errno));
		err_count++;
		return;
	}

	printf("\tattr_flag=%d ptr=%p\n", a.attr_flag, a.attr_un.au_char);
	for (s=a.attr_un.au_char; s && *s; s += strlen(s) + 1) {
		printf("\t'%s'\n", s);
	}

	free(a.attr_un.au_char);

	/* try an invalid table */
	printf("Trying an invalid table\n");
	ret = methods->method_getentry("ALL", INVALID_STRING, &attribs, &a, 1);
	if (ret == 0 || errno != ENOSYS) {		
		printf("ERROR: method_getentry expected\n\t%d/%s\n", -1, strerror(ENOSYS));
		printf("got\t%d/%s\n", ret, strerror(errno));
		err_count++;
	}

	printf("Trying an invalid user\n");
	ret = methods->method_getentry(INVALID_STRING, "user", &attribs, &a, 1);
	if (ret == 0 || errno != ENOENT) {		
		printf("ERROR: method_getentry expected\n\t%d/%s\n", -1, strerror(ENOENT));
		printf("got\t%d/%s\n", ret, strerror(errno));
		err_count++;
	}
	
}

/*
  test listing groups
*/
static void test_grouplist(void)
{
	int ret;
	attrval_t a;
	char *attribs = "groups";
	char *s;

	if (!methods->method_getentry) {
		printf("WARNING: module has no method_getentry\n");
		return;
	}

	printf("\ntesting method_getentry(ALL, group)\n");

	ret = methods->method_getentry("ALL", "group", &attribs, &a, 1);
	if (ret != 0) {
		printf("ERROR: method_getentry failed ret=%d - %s\n", ret, strerror(errno));
		err_count++;
		return;
	}

	printf("\tattr_flag=%d ptr=%p\n", a.attr_flag, a.attr_un.au_char);
	for (s=a.attr_un.au_char; s && *s; s += strlen(s) + 1) {
		printf("\t'%s'\n", s);
	}

	free(a.attr_un.au_char);


	printf("Trying an invalid group\n");
	ret = methods->method_getentry(INVALID_STRING, "group", &attribs, &a, 1);
	if (ret == 0 || errno != ENOENT) {		
		printf("ERROR: method_getentry expected\n\t%d/%s\n", -1, strerror(ENOENT));
		printf("got\t%d/%s\n", ret, strerror(errno));
		err_count++;
	}
}


static void show_pwd(struct passwd *pwd)
{
	printf("\t%s:%s:%d:%d:%s:%s:%s\n", 
	       pwd->pw_name,
	       pwd->pw_passwd,
	       pwd->pw_uid,
	       pwd->pw_gid,
	       pwd->pw_gecos,
	       pwd->pw_dir,
	       pwd->pw_shell);
}

static void free_pwd(struct passwd *pwd)
{
	free(pwd->pw_name);
	free(pwd->pw_passwd);
	free(pwd->pw_gecos);
	free(pwd->pw_dir);
	free(pwd->pw_shell);
	free(pwd);
}


static void show_grp(struct group *grp)
{
	int i;

	printf("\t%s:%s:%d: ", 
	       grp->gr_name,
	       grp->gr_passwd,
	       grp->gr_gid);
	
	if (!grp->gr_mem[0]) {
		printf("\n");
		return;
	}
	
	for (i=0; grp->gr_mem[i+1]; i++) {
		printf("%s, ", grp->gr_mem[i]);
	}
	printf("%s\n", grp->gr_mem[i]);
}

static void free_grp(struct group *grp)
{
	int i;

	free(grp->gr_name);
	if (!getenv("DONT_FREE_GR_PASSWD")) {
		free(grp->gr_passwd);
	}
	
	if (!grp->gr_mem) {
		free(grp);
		return;
	}
	
	for (i=0; grp->gr_mem[i]; i++) {
		free(grp->gr_mem[i]);
	}

	free(grp->gr_mem);
	free(grp);
}


/*
  test method_getpwnam
*/
static void test_getpwnam(char *name)
{
	struct passwd *pwd;

	printf("\ntesting method_getpwnam for user '%s'\n", name);

	pwd = methods->method_getpwnam(name);

	if (strcmp(name, INVALID_STRING) == 0) {
		if (pwd || errno != ENOENT) {
			printf("ERROR: method_getpwnam expected\n\t%p/%s\n", 0, strerror(ENOENT));
			printf("got\t%p/%s\n", pwd, strerror(errno));
			err_count++;
		}
		return;
	}
	
	if (!pwd) {
		printf("ERROR: method_getpwnam failed - %s\n", strerror(errno));
		err_count++;
		return;
	}

	show_pwd(pwd);
	free_pwd(pwd);
}

/*
  test method_getpwuid
*/
static void test_getpwuid(int uid)
{
	struct passwd *pwd;

	printf("\ntesting method_getpwuid for uid %d\n", uid);

	pwd = methods->method_getpwuid(uid);

	if (uid == INVALID_INT) {
		if (pwd || errno != ENOENT) {
			printf("ERROR: method_getpwuid expected\n\t%p/%s\n", 0, strerror(ENOENT));
			printf("got\t%p/%s\n", pwd, strerror(errno));
			err_count++;
		}
		return;
	}	

	if (!pwd) {
		printf("ERROR: method_getpwuid failed - %s\n", strerror(errno));
		err_count++;
		return;
	}

	show_pwd(pwd);
	free_pwd(pwd);
}


/*
  test method_getgrnam
*/
static void test_getgrnam(char *name)
{
	struct group *grp;

	printf("\ntesting method_getgrnam for group '%s'\n", name);

	grp = methods->method_getgrnam(name);
	if (strcmp(name, INVALID_STRING) == 0) {
		if (grp || errno != ENOENT) {
			printf("ERROR: method_getgrnam expected\n\t%p/%s\n", 0, strerror(ENOENT));
			printf("got\t%p/%s\n", grp, strerror(errno));
			err_count++;
		}
		return;
	}


	if (!grp) {
		printf("ERROR: method_getgrnam failed - %s\n", strerror(errno));
		err_count++;
		return;
	}

	show_grp(grp);
	free_grp(grp);
}

/*
  test method_getgracct
*/
static void test_getgracct(char *name)
{
	struct group *grp;
	int gid;

	printf("\ntesting method_getgracct for group '%s'\n", name);

	grp = methods->method_getgracct((void *)name, 1);

	if (strcmp(name, INVALID_STRING) == 0) {
		if (grp || errno != ENOENT) {
			printf("ERROR: method_getgracct expected\n\t%p/%s\n", 0, strerror(ENOENT));
			printf("got\t%p/%s\n", grp, strerror(errno));
			err_count++;
		}
		return;
	}

	if (!grp) {
		printf("ERROR: method_getgracct(name, 1) failed - %s\n", strerror(errno));
		err_count++;
		return;
	}

	gid = grp->gr_gid;
	show_grp(grp);
	free_grp(grp);

	grp = methods->method_getgracct((void *)&gid, 0);
	if (!grp) {
		printf("ERROR: method_getgracct(gid, 0) failed - %s\n", strerror(errno));
		err_count++;
		return;
	}

	show_grp(grp);
	free_grp(grp);
}

/*
  test method_getgrset
*/
static void test_getgrset(char *name)
{
	char *grps;

	printf("\ntesting method_getgrset for user '%s'\n", name);

	grps = methods->method_getgrset(name);

	if (strcmp(name, INVALID_STRING) == 0) {
		if (grps || errno != ENOENT) {
			printf("ERROR: method_getgrset expected\n\t%p/%s\n", 0, strerror(ENOENT));
			printf("got\t%p/%s\n", grps, strerror(errno));
			err_count++;
		}
		return;
	}

	if (!grps) {
		printf("ERROR: method_getgrset failed - %s\n", strerror(errno));
		err_count++;
		return;
	}

	printf("\t%s\n", grps);

	free(grps);
}

/*
  test method_normalize
*/
static void test_normalize(char *name)
{
	int ret;
	char shortname[S_NAMELEN];

	printf("\ntesting method_normalize for user '%s' (namelen=%d)\n", 
	       name, S_NAMELEN);

	ret = methods->method_normalize(name, shortname);

	if (strcmp(name, INVALID_STRING) == 0) {
		/* there seems to be no defined behaviour for this?! */
		return;
	}

	if (ret == 0) {
		printf("ERROR: method_normalize failed - %s\n", strerror(errno));
		err_count++;
		return;
	}

	printf("\t%s\n", shortname);
}

/*
  test method_getgrgid
*/
static void test_getgrgid(int gid)
{
	struct group *grp;

	printf("\ntesting method_getgrgid for gid %d\n", gid);

	grp = methods->method_getgrgid(gid);
	if (gid == INVALID_INT) {
		if (grp || errno != ENOENT) {
			printf("ERROR: method_getgrgid expected\n\t%p/%s\n", 0, strerror(ENOENT));
			printf("got\t%p/%s\n", grp, strerror(errno));
			err_count++;
		}
		return;
	}

	if (!grp) {
		printf("ERROR: method_getgrgid failed - %s\n", strerror(errno));
		err_count++;
		return;
	}

	show_grp(grp);
	free_grp(grp);
}


/*
  loop over all users
*/
static void forallusers(void (*fn)(char *name))
{
	int ret;
	attrval_t a;
	char *attribs = "users";
	char *s;

	if (!methods->method_getentry) {
		return;
	}
	ret = methods->method_getentry("ALL", "user", &attribs, &a, 1);
	if (ret != 0) {
		return;
	}
	for (s=a.attr_un.au_char; s && *s; s += strlen(s) + 1) {
		fn(s);
	}

	free(a.attr_un.au_char);

	fn(INVALID_STRING);
}


/*
  loop over all groups
*/
static void forallgroups(void (*fn)(char *name))
{
	int ret;
	attrval_t a;
	char *attribs = "groups";
	char *s;

	if (!methods->method_getentry) {
		return;
	}
	ret = methods->method_getentry("ALL", "group", &attribs, &a, 1);
	if (ret != 0) {
		return;
	}
	for (s=a.attr_un.au_char; s && *s; s += strlen(s) + 1) {
		fn(s);
	}

	free(a.attr_un.au_char);

	fn(INVALID_STRING);
}

/*
  loop over all uids
*/
static void foralluids(void (*fn)(int uid))
{
	int ret;
	attrval_t a;
	char *attribs = "users";
	char *s;

	if (!methods->method_getentry) {
		return;
	}
	ret = methods->method_getentry("ALL", "user", &attribs, &a, 1);
	if (ret != 0) {
		return;
	}
	for (s=a.attr_un.au_char; s && *s; s += strlen(s) + 1) {
		attrval_t a2;
		char *id_attribs = S_ID;
		ret = methods->method_getentry(s, "user", &id_attribs, &a2, 1);
		if (ret != 0) {
			printf("ERROR: failed to get S_ID for '%s' - ret=%d - %s\n", 
			       s, ret, strerror(errno));
			err_count++;
		}
		fn(a2.attr_un.au_int);
	}

	free(a.attr_un.au_char);

	fn(INVALID_INT);
}

/*
  loop over all gids
*/
static void forallgids(void (*fn)(int gid))
{
	int ret;
	attrval_t a;
	char *attribs = "groups";
	char *s;

	if (!methods->method_getentry) {
		return;
	}
	ret = methods->method_getentry("ALL", "group", &attribs, &a, 1);
	if (ret != 0) {
		return;
	}
	for (s=a.attr_un.au_char; s && *s; s += strlen(s) + 1) {
		attrval_t a2;
		char *id_attribs = S_ID;
		ret = methods->method_getentry(s, "group", &id_attribs, &a2, 1);
		if (ret != 0) {
			printf("ERROR: failed to get S_ID for '%s' - ret=%d - %s\n", 
			       s, ret, strerror(errno));
			err_count++;
		}
		fn(a2.attr_un.au_int);
	}

	free(a.attr_un.au_char);

	fn(INVALID_INT);
}


/*
  the main test harness 
*/
static void uess_test(void)
{
	test_attrlist();

	test_userlist();

	test_grouplist();

	if (methods->method_getpwnam) {
		forallusers(test_getpwnam);
	}
	if (methods->method_getpwuid) {
		foralluids(test_getpwuid);
	}
	if (methods->method_getgrset) {
		forallusers(test_getgrset);
	}
	if (methods->method_normalize) {
		forallusers(test_normalize);
	}
	if (methods->method_getgrnam) {
		forallgroups(test_getgrnam);
	}
	if (methods->method_getgracct) {
		forallgroups(test_getgracct);
	}
	if (methods->method_getgrgid) {
		forallgids(test_getgrgid);
	}
}


/*
  return the current size of the heap (in blocks)
*/
static unsigned long heap_size(void)
{
	struct mallinfo m = mallinfo();
	return m.ordblks + m.smblks;
}


struct secmethod_desc {
	char *name;
	char *module_path;
	struct secmethod_table *methods;
};

extern struct secmethod_desc *_load_secmethod(const char *name);

int main(int argc, const char *argv[])
{
	const char *name;
	struct secmethod_desc *r;
	int i, nloops = 1;
	unsigned long prev_alloc = 0;

	if (argc < 2) {
		printf("Usage: uess_test <MODULE_NAME> <nloops>\n");
		exit(1);
	}

	name = argv[1];

	if (argc > 2) {
		nloops = atoi(argv[2]);
	}

	errno = 0;

	r = _load_secmethod(name);

	if (!r) {
		printf("Unable to load module '%s'\n", name);
		exit(1);
	}

	printf("Loaded '%s' with path '%s'\n", r->name, r->module_path);
	printf("method_version=%d\n", r->methods->method_version);

	methods = r->methods;

	for (i=0;i<nloops; i++) {
		unsigned long hsize;

		printf("Test loop %d\n", i);

		uess_test();

		hsize = heap_size();

		if (prev_alloc && hsize > prev_alloc) {
			printf("ERROR: heap grew by %d blocks. Probable memory leak\n", 
			       hsize - prev_alloc);
			err_count++;
		}
		prev_alloc = hsize;
	}

	if (err_count != 0) {
		printf("WARNING: %d errors detected\n", err_count);
	}

	return err_count;
}
