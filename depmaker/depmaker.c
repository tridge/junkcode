/* a hacking I will go ....

   This preload allows you to build a dependency list for a build
   
   tridge@linuxcare.com
   October 2000
 */

#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <linux/cdrom.h>

#define O_APPEND 02000
#define O_CREAT  0100
#define O_WRONLY 01 
#define O_RDONLY 0

extern char *getcwd(char *, size_t );
extern ssize_t write(int , const void *, size_t );
extern ssize_t read(int , void *, size_t );
extern pid_t getpid(void);
extern int sprintf(char *, const char *, ...);

static size_t strlcat(char *d, const char *s, size_t bufsize)
{
	size_t len1 = strlen(d);
	size_t len2 = strlen(s);
	size_t ret = len1 + len2;

	if (len1+len2 >= bufsize) {
		len2 = bufsize - (len1+1);
	}
	if (len2 > 0) {
		memcpy(d+len1, s, len2);
		d[len1+len2] = 0;
	}
	return ret;
}

static void logit(const char *filename, int addcwd)
{
	static int logfd = -1;
	char *p = getenv("DEPMAKER");
	char buf[1024];
	char buf2[1024];

	if (!p) return;

	if (logfd == -1) {
		int (*ropen)(const char *, int , int );
		ropen = dlsym((void *)-1, "open");
		logfd = ropen(p, O_WRONLY|O_CREAT|O_APPEND, 0644);
	}

	buf[0] = 0;
	if (addcwd && filename[0] != '/') {
		getcwd(buf2, sizeof(buf2));
		strlcat(buf,buf2,sizeof(buf));
		strlcat(buf,"/",sizeof(buf));
	}
	strlcat(buf, filename, sizeof(buf));
	strlcat(buf, "\n", sizeof(buf));
	write(logfd, buf, strlen(buf));
}

#define WRAP(fn, TEST)						\
int fn(const char *filename, int a1, int a2, int a3) {		\
	int ret;						\
	static int (*real)(const char *, int , int, int );	\
	if (!real) real = dlsym((void *)-1, #fn);		\
	ret = real(filename, a1, a2, a3);			\
	if (ret != TEST) logit(filename, 1);			\
	return ret;						\
}

WRAP(open, -1)
WRAP(open64, -1)
WRAP(opendir, 0)
WRAP(opendir64, 0)
WRAP(fopen, 0)
WRAP(access, -1)
WRAP(stat, -1)
WRAP(lstat, -1)
WRAP(stat64, -1)
WRAP(lstat64, -1)

/* this handles exec calls */
void _init(void)
{
	char buf[1024];
	int (*ropen)(const char *, int , int );
	int fd, i, len;

	if (realpath("/proc/self/exe", buf)) {
		logit(buf, 1);
	}

	ropen = dlsym((void *)-1, "open");

	fd = ropen("/proc/self/cmdline", O_RDONLY, 0);

	strcpy(buf,"COMMAND: ");
	len = read(fd, buf+9, sizeof(buf)-10);
	for (i=0;i<len;i++) if (buf[9+i] == 0) buf[9+i] = ' ';
	buf[9+len] = 0;
	logit(buf, 0);
}
