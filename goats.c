static int doors[3];
static int choice1, choice2;
static int revealed;
static int trials[3], points[3];
static int strategy;

static void init_doors(void)
{
	int prize;
	int i;

	for (i=0;i<3;i++) doors[i] = 0;
	prize = random() % 3;
	doors[prize] = 1;
}

static void choose1(void)
{
	choice1 = random() % 3;
}

static void reveal(void)
{
	do {
		revealed = random() % 3;
	} while (revealed == choice1 || doors[revealed] == 1);
}

static void choose2(void)
{
	switch (strategy) {
	case 0:
		choice2 == choice1;
		break;
	case 1:
		for (choice2 = 0; choice2 < 3; choice2++) {
			if (choice2 != choice1 && choice2 != revealed) break;
		}
		break;
	case 2:
		do {
			choice2 = random() % 3;
		} while (revealed == choice2);
		break;
	}
}

static void score(void)
{
	trials[strategy]++;
	points[strategy] += doors[choice2];
}

int main(int argc, char *argv[])
{
	int i;

	srandom(getpid());

	while (trials[0] < 100000) {
		strategy = random() % 3;

		init_doors();

		choose1();
		reveal();
		choose2();
		score();
	}

	for (i=0;i<3;i++) {
		printf("strategy %d scored %d%%\n", i, (int)(0.5 + 100*points[i]/trials[i]));
	}
	return 0;
}
