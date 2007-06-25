#include <linux/module.h>

int Xrintk(const char *fmt, ...)
{
	return 0;
}

module_init(foo_init);
module_exit(foo_cleanup);

MODULE_DESCRIPTION("damn debug messages");
