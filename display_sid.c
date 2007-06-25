/*
  display a binary sid

  assumes x86 CPU. This code is NOT portable.

  tridge@samba.org, December 2003
*/

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>


typedef unsigned char uint8;
typedef unsigned int  uint32;

struct dom_sid {
#if CONFORMANT_SID
	uint32  conf_size;
#endif
	uint8  sid_rev_num;
	uint8  num_auths;
	uint8  id_auth[6];
	uint32 sub_auths[0];
};


static void display_sid(const char *blob, off_t size)
{
	struct dom_sid *sid = (struct dom_sid *)blob;
	uint32 ia;
	int i;

	if (size < sizeof(*sid) ||
#if CONFORMANT_SID
	    sid->conf_size != sid->num_auths ||
#endif
	    size != sizeof(*sid) + 4*sid->num_auths) {
		printf("Bad sid blob - maybe its not a IDL conformant SID?\n");
		exit(1);
	}

	ia = (sid->id_auth[5]) +
		(sid->id_auth[4] << 8 ) +
		(sid->id_auth[3] << 16) +
		(sid->id_auth[2] << 24);

	printf("S-%u-%u", (unsigned)sid->sid_rev_num, (unsigned)ia);

	for (i=0;i<sid->num_auths;i++) {
		printf("-%u", (unsigned)sid->sub_auths[i]);
	}
	printf("\n");
}


static const char *file_load(const char *filename, off_t *size)
{
	char *ret;
	struct stat st;
	int fd;

	fd = open(filename, O_RDONLY);
	if (fd == -1) return NULL;

	fstat(fd, &st);
	
	ret = malloc(st.st_size);
	if (!ret) return NULL;

	if (read(fd, ret, st.st_size) != st.st_size) {
		free(ret);
		return NULL;
	}

	*size = st.st_size;

	return ret;
}

int main(int argc, const char *argv[])
{
	const char *filename;
	const char *blob;
	off_t size;

	if (argc < 2) {
		fprintf(stderr,"Usage: display_sid <filename>\n");
		exit(1);
	}
	
	filename = argv[1];

	blob = file_load(filename, &size);
	if (!blob) {
		perror(filename);
		exit(1);
	}

	display_sid(blob, size);
	return 0;
}

