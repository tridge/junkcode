/*
  preload hack to allow programs be distribution agnostic

  This is like LD_LIBRARY_PATH, but works for files

  tridge@samba.org March 2007
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <setjmp.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

static int hack_disabled = -1;

static const char *skip_paths[] = {
	"/opt/ibm",
	"notesplugin",
	"sametime",
	"noteshack",
	NULL
};

/*
  catch segv when SOFTROOT_LOG is set
*/
static int segv_handler(int n)
{
	char *cmd;
	asprintf(&cmd, "xterm -e gdb /proc/%d/exe %d", getpid(), getpid());
	system(cmd);
	return 0;
}

static void xlogit(const char *fmt, ...)
{
	static int initialised;
	static int logfd = -1;
	va_list ap;
	static int (*open_orig)(const char *, int, mode_t);
	static char *(*getenv_orig)(const char *);

	if (logfd == -1 && !initialised) {
		const char *log_name;
		initialised = 1;
		getenv_orig = dlsym(RTLD_NEXT, "getenv");
		open_orig = dlsym(RTLD_NEXT, "open");
		log_name = getenv_orig("SOFTROOT_LOG");
		if (log_name != NULL) {
			logfd = open_orig(log_name, O_WRONLY|O_APPEND|O_CREAT, 0666);
		}
	}

	if (logfd != -1) {
		signal(SIGSEGV, (sighandler_t)segv_handler);
		va_start(ap, fmt);
		vdprintf(logfd, fmt, ap);
		va_end(ap);
	}
}


static const char *redirect_name(const char *filename)
{
	static char buf[1024];
	static int (*access_orig)(const char *, int );
	static char *(*getenv_orig)(const char *);
	static const char *lroot;

	if (filename == NULL) return filename;

	if (!getenv_orig) {
		getenv_orig = dlsym(RTLD_NEXT, "getenv");
	}

	if (hack_disabled == -1) {
		if (getenv_orig("SOFTROOT_DISABLED")) {
			hack_disabled = 1;
		} else {
			hack_disabled = 0;
		}
	}

	if (hack_disabled) {
		return filename;
	}

	if (lroot == NULL) {
		lroot = getenv_orig("LROOT");
	}
	if (lroot == NULL) {
		lroot = "/opt/softroot";
	}
	if (filename[0] != '/' || strncmp(filename, lroot, strlen(lroot)) == 0) {
		return filename;
	}
	if (!access_orig) {
		access_orig = dlsym(RTLD_NEXT, "access");
	}
	snprintf(buf, sizeof(buf), "%s%s", lroot, filename);
	if (access_orig(buf, F_OK) == -1) {
		return filename;
	}
	
	xlogit("mapped '%s' to '%s'\n", filename, buf);

	return buf;
}

int setenv(const char *name, const char *value, int overwrite)
{
	static int (*setenv_orig)(const char *, const char *, int);
	int ret;

	if (!setenv_orig) {
		setenv_orig = dlsym(RTLD_NEXT, "setenv");
	}

	/*
	  don't let progs install their own preload or
	  preload_softroot.so would be lost
	*/
	if (strcmp("LD_PRELOAD", name) == 0) {
		xlogit("skipping LD_PRELOAD of '%s'\n", value);
		return 0;
	}

	ret = setenv_orig(name, value, overwrite);

	xlogit("setenv(\"%s\", \"%s\", %d) -> %d\n", 
		name, value, overwrite, ret);

	return ret;
}

char *getenv(const char *name)
{
	static char *(*getenv_orig)(const char *);
	char *ret;

	if (!getenv_orig) {
		getenv_orig = dlsym(RTLD_NEXT, "getenv");
	}

	ret = getenv_orig(name);

	xlogit("getenv(\"%s\") -> '%s'\n", name, ret);

	return ret;
}

void *dlopen(const char *filename, int flag)
{
	static void *(*dlopen_orig)(const char *, int);
	void *ret;

	if (!dlopen_orig) {
		dlopen_orig = dlsym(RTLD_NEXT, "dlopen");
	}

	ret = dlopen_orig(redirect_name(filename), flag);

	xlogit("dlopen(\"%s\", 0x%x) -> %p\n", filename, flag, ret);

	return ret;	
}


int open(const char *filename, int flags, ...)
{
	static int (*open_orig)(const char *, int, mode_t);
	int ret;
	va_list ap;
	mode_t mode;

	if (!open_orig) {
		open_orig = dlsym(RTLD_NEXT, "open");
	}

	va_start(ap, flags);
	mode = va_arg(ap, mode_t);
	va_end(ap);

	ret = open_orig(redirect_name(filename), flags, mode);

	xlogit("open(\"%s\", 0x%x, %o) -> %d\n", filename, flags, mode, ret);

	return ret;	
}

int open64(const char *filename, int flags, ...)
{
	static int (*open64_orig)(const char *, int, mode_t);
	int ret;
	va_list ap;
	mode_t mode;

	if (!open64_orig) {
		open64_orig = dlsym(RTLD_NEXT, "open64");
	}

	va_start(ap, flags);
	mode = va_arg(ap, mode_t);
	va_end(ap);

	ret = open64_orig(redirect_name(filename), flags, mode);

	xlogit("open64(\"%s\", 0x%x, %o) -> %d\n", filename, flags, mode, ret);

	return ret;	
}

int __open(const char *filename, int flags, ...)
{
	static int (*__open_orig)(const char *, int, mode_t);
	int ret;
	va_list ap;
	mode_t mode;

	if (!__open_orig) {
		__open_orig = dlsym(RTLD_NEXT, "__open");
	}

	va_start(ap, flags);
	mode = va_arg(ap, mode_t);
	va_end(ap);

	ret = __open_orig(redirect_name(filename), flags, mode);

	xlogit("__open(\"%s\", 0x%x, %o) -> %d\n", filename, flags, mode, ret);

	return ret;	
}

int __open64(const char *filename, int flags, ...)
{
	static int (*__open64_orig)(const char *, int, mode_t);
	int ret;
	va_list ap;
	mode_t mode;

	if (!__open64_orig) {
		__open64_orig = dlsym(RTLD_NEXT, "__open64");
	}

	va_start(ap, flags);
	mode = va_arg(ap, mode_t);
	va_end(ap);

	ret = __open64_orig(redirect_name(filename), flags, mode);

	xlogit("__open64(\"%s\", 0x%x, %o) -> %d\n", filename, flags, mode, ret);

	return ret;	
}

int stat(const char *filename, struct stat *st)
{
	static int (*stat_orig)(const char *, struct stat *);
	int ret;

	if (!stat_orig) {
		stat_orig = dlsym(RTLD_NEXT, "stat");
	}

	ret = stat_orig(redirect_name(filename), st);

	xlogit("stat(\"%s\") -> %d\n", filename, ret);

	return ret;	
}

int stat64(const char *filename, struct stat64 *st)
{
	static int (*stat64_orig)(const char *, struct stat64 *);
	int ret;

	if (!stat64_orig) {
		stat64_orig = dlsym(RTLD_NEXT, "stat64");
	}

	ret = stat64_orig(redirect_name(filename), st);

	xlogit("stat64(\"%s\") -> %d\n", filename, ret);

	return ret;	
}

FILE *fopen(const char *filename, const char *mode)
{
	static FILE *(*fopen_orig)(const char *, const char *);
	FILE *ret;

	if (!fopen_orig) {
		fopen_orig = dlsym(RTLD_NEXT, "fopen");

	}

	ret = fopen_orig(redirect_name(filename), mode);

	xlogit("fopen(\"%s\", \"%s\") -> %p\n", filename, mode, ret);

	return ret;	
}

void *gzopen(const char *filename, const char *mode)
{
	static void *(*gzopen_orig)(const char *, const char *);
	void *ret;

	if (!gzopen_orig) {
		gzopen_orig = dlsym(RTLD_NEXT, "gzopen");
	}

	ret = gzopen_orig(redirect_name(filename), mode);

	xlogit("gzopen(\"%s\", \"%s\") -> %p\n", filename, mode, ret);

	return ret;	
}

DIR *opendir(const char *filename)
{
	static DIR *(*opendir_orig)(const char *);
	DIR *ret;

	if (!opendir_orig) {
		opendir_orig = dlsym(RTLD_NEXT, "opendir");
	}

	/* opendir is an exception - try the real dir first */
	ret = opendir_orig(filename);
	if (ret == NULL) {
		ret = opendir_orig(redirect_name(filename));
	}

	xlogit("opendir(\"%s\") -> %p\n", filename, ret);

	return ret;	
}

int __xstat(int x, const char *filename, struct stat *st)
{
	static int (*__xstat_orig)(int x, const char *, struct stat *);
	int ret;

	if (!__xstat_orig) {
		__xstat_orig = dlsym(RTLD_NEXT, "__xstat");
	}

	ret = __xstat_orig(x, redirect_name(filename), st);

	xlogit("__xstat(\"%s\") -> %d\n", filename, ret);

	return ret;	
}

int __xstat64(int x, const char *filename, struct stat64 *st)
{
	static int (*__xstat64_orig)(int x, const char *, struct stat64 *);
	int ret;

	if (!__xstat64_orig) {
		__xstat64_orig = dlsym(RTLD_NEXT, "__xstat64");
	}

	ret = __xstat64_orig(x, redirect_name(filename), st);

	xlogit("__xstat64(\"%s\") -> %d\n", filename, ret);

	return ret;	
}

int access(const char *filename, int x)
{
	static int (*access_orig)(const char *, int );
	int ret;

	if (!access_orig) {
		access_orig = dlsym(RTLD_NEXT, "access");
	}

	ret = access_orig(redirect_name(filename), x);

	xlogit("access(\"%s\") -> %d\n", filename, ret);

	return ret;	
}


static void check_preload_disable(const char *filename)
{
	int i;
	int (*setenv_orig)(const char *, const char *, int);
	char *(*getenv_orig)(const char *);
	setenv_orig = dlsym(RTLD_NEXT, "setenv");
	getenv_orig = dlsym(RTLD_NEXT, "getenv");
	int is_external_path = 1;

	for (i=0;skip_paths[i];i++) {
		if (strstr(filename, skip_paths[i])) {
			is_external_path = 0;
		}
	}

	if (!hack_disabled && is_external_path) {
		hack_disabled = 1;
		xlogit("disabling preload for '%s'\n", filename);
		setenv_orig("SOFTROOT_DISABLED", getenv_orig("LD_LIBRARY_PATH"), 1);
		unsetenv("LD_LIBRARY_PATH");
	} else if (hack_disabled && !is_external_path) {
		hack_disabled = 0;
		xlogit("enabling preload for '%s'\n", filename);
		setenv_orig("LD_LIBRARY_PATH", getenv_orig("SOFTROOT_DISABLED"), 1);
		unsetenv("SOFTROOT_DISABLED");
	}
}


int execve(const char *filename, char *const argv[], char *const envp[])
{
	static int (*execve_orig)(const char *, char *const argv[], char *const envp[] );

	if (!execve_orig) {
		execve_orig = dlsym(RTLD_NEXT, "execve");
	}

	filename = redirect_name(filename);
	check_preload_disable(filename);

	xlogit("execve(\"%s\")\n", filename);

	return execve_orig(filename, argv, envp);
}


int execvp(const char *filename, char *const argv[])
{
	static int (*execvp_orig)(const char *, char *const argv[]);

	if (!execvp_orig) {
		execvp_orig = dlsym(RTLD_NEXT, "execvp");
	}

	filename = redirect_name(filename);
	check_preload_disable(filename);

	xlogit("execvp(\"%s\")\n", filename);

	return execvp_orig(filename, argv);
}


int execv(const char *filename, char *const argv[])
{
	static int (*execv_orig)(const char *, char *const argv[]);

	if (!execv_orig) {
		execv_orig = dlsym(RTLD_NEXT, "execv");
	}

	filename = redirect_name(filename);
	check_preload_disable(filename);

	xlogit("execv(\"%s\")\n", filename);

	return execv_orig(filename, argv);
}


static int is_date_list(const void *src, int n)
{
	const unsigned short *uc = (unsigned short *)src;
	int i;

	if (n < 42 || n & 1) return 0;

	if (uc[2] != '/') return 0;

	n /= 2;

	for (i=0;i<n;i+=11) {
		if (uc[i] > 255) return 0;
	}

	for (i=0;i<n;i+=11) {
		if (!isdigit(uc[i]) ||
		    !isdigit(uc[i+1]) ||
		    uc[i+2] != '/' ||
		    !isdigit(uc[i+3]) ||
		    !isdigit(uc[i+4]) ||
		    uc[i+5] != '/' ||
		    !isdigit(uc[i+6]) ||
		    !isdigit(uc[i+7]) ||
		    !isdigit(uc[i+8]) ||
		    !isdigit(uc[i+9])) return 0;
		if (n-i > 10 && uc[i+10] != '\n') return 0;
	}
	
	return 1;
}

/*
  this memmove hook is needed for the Australian employee workplace
  database, which assumes date strings are in \r\n format
*/
void *memmove(void *dest, const void *src, size_t n)
{
	static void *(*memmove_orig)(void *, const void *, size_t);
	void *ret;

	if (!memmove_orig) {
		memmove_orig = dlsym(RTLD_NEXT, "memmove");
	}

	ret = memmove_orig(dest, src, n);

	if (is_date_list(dest, n)) {
		unsigned char *d;
		const unsigned char *s;
		size_t n2 = n;
		d = dest;
		s = src;
		xlogit("Found date list\n");
		while (n2--) {
			if (*s == '\n') {
				/* to fit the CRLF into the string, we
				   rely on the fact that date strings
				   are given in dd/mm/yyyy format, but
				   the target functions also handle the
				   dd/mm/yy format, so we have 4 bytes
				   extra to play with per date */
				d -= 8;
				s -= 4;
				if ((d[0] == '2') && (d[2] == '9')) {
					d += 8;
					s += 4;
				} else {
					*d++ = *s++;
					*d++ = *s++;
					*d++ = *s++;
					*d++ = *s++;
				}
				*d++ = '\r';
				*d++ = 0;
			}
			*d++ = *s++;
		}
		/* note that we don't adjust the last date, as the app
		 * needs to see final dates of the form dd/mm/2999 as
		 * a terminator */
		*d++ = 0;
		*d++ = 0;
		/* this is the really nasty bit. We've fixed the
		 * string, but now we must fix the source string as
		 * well! This is needed as the caller is really the
		 * one that needs the update, but we can't hook the
		 * caller directly due to inline functions. */
		memmove_orig(src, dest, n);
	}

	return ret;
}

void *memcpy(void *dest, const void *src, size_t n)
{
	return memmove(dest, src, n);
}


void bcopy(const void *src, void *dest, size_t n)
{
	memmove(dest, src, n);
}

/*
  quite a few NSD errors, particularly in printing, are caused by
  strcmp() calls with NULL arguments. This makes strcmp() more robust
*/
int strcmp(const char *s1, const char *s2)
{
	static int (*strcmp_orig)(const char *, const char *);

	if (!strcmp_orig) {
		strcmp_orig = dlsym(RTLD_NEXT, "strcmp");
	}

	if (s1 == NULL || s2 == NULL) {
		xlogit("caught strcmp with NULL\n");
	}

	if (s1 == s2) return 0;
	if (s1 == NULL) return -1;
	if (s2 == NULL) return 1;

	return strcmp_orig(s1, s2);
}

#if 1
/*
  catch X events, to prevent the CWStackMode bit from causing troubles
*/
int XNextEvent(Display *display, XEvent *event_return)
{
	static int (*XNextEvent_orig)(Display *, XEvent *);
	int ret;

	if (!XNextEvent_orig) {
		XNextEvent_orig = dlsym(RTLD_NEXT, "XNextEvent");
	}

	ret = XNextEvent_orig(display, event_return);

	xlogit("event_type=%d ret=%d\n", event_return->type, ret);

	if (ret == True && event_return->type == ConfigureRequest) {
		xlogit("value_mask=0x%x\n",
		       event_return->xconfigurerequest.value_mask);
	}
	
	return ret;
}


/*
  catch X events, to prevent the CWStackMode bit from causing troubles
*/
int XCheckTypedEvent(Display *display, int event_type, XEvent *event_return)
{
	static int (*XCheckTypedEvent_orig)(Display *, int, XEvent *);
	int ret;

	if (!XCheckTypedEvent_orig) {
		XCheckTypedEvent_orig = dlsym(RTLD_NEXT, "XCheckTypedEvent");
	}

	ret = XCheckTypedEvent_orig(display, event_type, event_return);

	if (ret == True) {
		xlogit("checked event_type=%d ret=%d\n", event_return->type, ret);
	}

	return ret;
}

/*
  catch X events, to prevent the CWStackMode bit from causing troubles
*/
int XCheckTypedWindowEvent(Display *display, Window w, int event_type, XEvent *event_return)
{
	static int (*XCheckTypedWindowEvent_orig)(Display *, Window, int, XEvent *);
	int ret;

	if (!XCheckTypedWindowEvent_orig) {
		XCheckTypedWindowEvent_orig = dlsym(RTLD_NEXT, "XCheckTypedWindowEvent");
	}

	ret = XCheckTypedWindowEvent_orig(display, w, event_type, event_return);

	if (ret == True) {
		xlogit("checked window event_type=%d ret=%d\n", event_return->type, ret);
	}

	return ret;
}

#endif

#define DWORD unsigned
#define WORD unsigned short
#define LONG int
#define BYTE unsigned char

typedef struct tagBITMAPINFOHEADER{
  DWORD  biSize; 
  LONG   biWidth; 
  LONG   biHeight; 
  WORD   biPlanes; 
  WORD   biBitCount; 
  DWORD  biCompression; 
  DWORD  biSizeImage; 
  LONG   biXPelsPerMeter; 
  LONG   biYPelsPerMeter; 
  DWORD  biClrUsed; 
  DWORD  biClrImportant; 
} BITMAPINFOHEADER, *PBITMAPINFOHEADER; 

typedef struct tagRGBQUAD {
  BYTE    rgbBlue; 
  BYTE    rgbGreen; 
  BYTE    rgbRed; 
  BYTE    rgbReserved; 
} RGBQUAD; 

typedef struct tagBITMAPINFO { 
  BITMAPINFOHEADER bmiHeader; 
  RGBQUAD          bmiColors[1]; 
} BITMAPINFO, *PBITMAPINFO; 

int _ZN6chamix15DrawableContext13StretchDIBitsEiiiiiiiiPKvPK13tagBITMAPINFOjm(
	void *hdc, 
	int XDest, 
	int YDest, 
	int nDestWidth, 
	int nDestHeight, 
	int XSrc, 
	int YSrc, 
	int nSrcWidth, 
	int nSrcHeight, 
	void const *lpBits, 
	BITMAPINFO const *lpBitsInfo, 
	unsigned int iUsage, 
	unsigned long dwRop)
{
	static int (*StretchDIBits_orig)(void *hdc, 
					 int XDest, 
					 int YDest, 
					 int nDestWidth, 
					 int nDestHeight, 
					 int XSrc, 
					 int YSrc, 
					 int nSrcWidth, 
					 int nSrcHeight, 
					 void const *lpBits, 
					 BITMAPINFO const *lpBitsInfo, 
					 unsigned int iUsage, 
					 unsigned long dwRop);
	int ret;

	if (!StretchDIBits_orig) {
		StretchDIBits_orig = dlsym(RTLD_NEXT, "_ZN6chamix15DrawableContext13StretchDIBitsEiiiiiiiiPKvPK13tagBITMAPINFOjm");
	}

	xlogit("StretchDIBits(%p, %d, %d, %d, %d, %d, %d, %d, %d, %p, %p, %u, %u)\n",
	       hdc, XDest, YDest, nDestWidth, nDestHeight, XSrc, YSrc, nSrcWidth, nSrcHeight, lpBits, lpBitsInfo, iUsage, dwRop);
	xlogit("StretchDIBits: %u %u %u %u %u %u %u %u %u %u %u\n",
	       lpBitsInfo->bmiHeader.biSize,
	       lpBitsInfo->bmiHeader.biWidth,
	       lpBitsInfo->bmiHeader.biHeight,
	       lpBitsInfo->bmiHeader.biPlanes,
	       lpBitsInfo->bmiHeader.biBitCount,
	       lpBitsInfo->bmiHeader.biCompression,
	       lpBitsInfo->bmiHeader.biSizeImage,
	       lpBitsInfo->bmiHeader.biXPelsPerMeter,
	       lpBitsInfo->bmiHeader.biYPelsPerMeter,
	       lpBitsInfo->bmiHeader.biClrUsed,
	       lpBitsInfo->bmiHeader.biClrImportant);

	/* avoid calls that crash with SIGFPE */
	if (labs(lpBitsInfo->bmiHeader.biHeight) <= 1) {
		xlogit("skipping StretchDIBits\n");
		return -1;
	}

	ret = StretchDIBits_orig(hdc, XDest, YDest, nDestWidth, nDestHeight, XSrc, YSrc,
				 nSrcWidth, nSrcHeight, lpBits, lpBitsInfo, iUsage, dwRop);
	return ret;	
}
