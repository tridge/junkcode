#include <stdio.h>


static void subone(char *buf, char *s, char c, int size)
{
	int l = strlen(s);
	while (size--) {
		if ((*buf) == (*s) && 
		    strcmp(buf, s) == 0 &&
		    (buf[-1] < 'a' || buf[-1] > 'z')) {
			buf[0] = c;
		}
		buf++;
	}
}

static void subit(char *buf, char *fname, char c, int size)
{
	FILE *f;
	char line[100];

	f = fopen(fname,"r");
	if (!f) exit(1);

	while (fgets(line, sizeof(line)-1, f)) {
		line[strlen(line)-1] = 0;
		if (strlen(line) > 1) {
			subone(buf, line, c, size);
		}
	}

	fclose(f);
}

main(int argc, char *argv[])
{
	char *subfile;
	char subchar;
	int size;
	static char buf[1024*1024*4];

	if (argc < 3) {
		fprintf(stderr,"read the code\n");
		exit(1);
	}

	subfile = argv[1];
	subchar = argv[2][0];

	while ((size = read(0, buf, sizeof(buf))) > 0) {
		subit(buf, subfile, subchar, size);
		
		write(1, buf, size);
	}
	exit(0);
}
