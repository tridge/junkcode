

struct UserStruct {
	char *UserName;
	uid_t uid;
	gid_t *gid_list;
	unsigned num_groups;
};


errid CreateUser(SysCtx *ctx, struct UserStruct *user);


main()
{
	struct UserStruct user;


	user.UserName = "bob";
	user.uid = 3;
	user.gid_list = SYS_malloc(ctx, sizeof(gid_t) * num_groups);
	user.num_groups = i;
	for (i=0;i<user.num_groups;i++) {
		user.gid[] = ;
	}

	CreateUser(ctx, &user);

}

main()
{
	struct UserStruct *user;

	SYY_EmptyUser(ctx,&user);
	strcpy(user->UserName, "bob");

	user.uid = 3;
	user.gid_list = SYS_malloc(ctx, sizeof(gid_t) * num_groups);
	user.gid_list = SYS_malloc(ctx, sizeof(gid_t) * num_groups);
	user.num_groups = i;
	for (i=0;i<user.num_groups;i++) {
		user.gid[] = ;
	}

	CreateUser(ctx, &user);

}
