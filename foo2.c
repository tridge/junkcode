
FILE *fopen(const char *fname, char *rw)
{

}

struct vnode *vn;

struct dentry *de;

void *opendir();


int myfunction(struct SysCtx *ctx, const char *name)




foo_fopen(FILE *f, char *rw)
{

}


foo_free() OR pools ?


main()
{
	FILE f;

	foo_fopen(&f, "w");

}

struct Sys_SrvName {
	char *srvname;
	char *adminname;
	char *location;
}

static void dans_set_ip(SysCtx *ctx, SYS_IP *ip)
{
	Sys_PushMemCtx(ctx);
	foo(ctx, ip);
	Sys_PopMemCtx(ctx);
}

void dans_func(void)
{
	struct Sys_SrvName *buf;
	SysCtx *ctx;

	ctx = Sys_InitCtx();

	Sys_Lock(ctx);
	Sys_GetSrvName(ctx, &buf);

	printf("Current is %s - %s - %s\n",
	       buf->srvname, buf->adminname, buf->location);

	Sys_Free(ctx, buf->location);

	ctx_asprintf(ctx, &buf->location, "newloc");
	ctx_asprintf(ctx, &buf->srvname, "%s - srvnamenew", buf->srvname);

	if (my_var > 3) {
		int fd;
		foo2();
		fd = Sys_OpenAlan(ctx, "/tmp/xxx",O_RDONLY);
		if (fd == -1) {
			return SYS_FAILURE;
		}
	}

	Sys_SetSrvName(ctx, buf);

	Sys_UnLock(ctx);

	Sys_CtxNew(ctx);
}


main()
{
	dans_func(ctx);
}
