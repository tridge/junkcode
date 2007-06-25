struct foo {
	char user[20];
	int x[10];
};

#define NUSERS 50000

struct foo users[NUSERS];

static int compar(struct foo *f1, struct foo *f2)
{
	return strcmp(f1->user, f2->user);
}

main()
{
	int i;
	for (i=0; i<NUSERS; i++) {
		sprintf(users[i].user, "USER%d", random());
	}
	qsort(users, NUSERS, sizeof(struct foo), compar);
}
