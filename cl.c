#include <errno.h>
#include <asm/unistd.h>

_syscall2(int, clone, int, flags, int, sp);


int libc_clone(int (*fn)(), void *child_stack, int flags)
{
	int ret;

	if (!child_stack) {
		errno = EINVAL;
		return -1;
	}

	ret = clone(flags, child_stack);
	if (ret != 0) return ret;

	ret = fn();
	exit(ret);
}

main()
{
	libc_clone(0,0);
}
