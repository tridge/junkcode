#include <errno.h>
#include <asm/unistd.h>
#include <asm/stat.h>

_syscall2(long, stat64, char *, fname, struct stat64 *, st)
