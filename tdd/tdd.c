/* simple dd with large offsets

   tridge@samba.org, September 2000

   released under the Gnu General Public License version 2 of later
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <asm/unistd.h>

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

static int sparse_writes;

_syscall5(int , _llseek, 
	  int, fd, 
	  unsigned long , offset_high,
	  unsigned long , offset_low,
	  loff_t *, result,
	  unsigned int, origin)

static loff_t llseek(int fd, loff_t offset, unsigned origin)
{
	loff_t ofs=0;
	int ret;

	ret = _llseek(fd, offset>>32, offset&0xffffffff, &ofs, origin);
	if (ret == 0) return ofs;
	return ret;
}

extern int ftruncate64(int , loff_t);


static loff_t getval(char *s)
{
	char *p;
	int i;
	loff_t ret;
	struct {
		char *s;
		loff_t m;
	} multipliers[] = {
		{"c", 1},
		{"w", 2},
		{"b", 512},
		{"kD", 1000},
		{"MD", 1000*1000},
		{"GD", 1000*1000*1000},
		{"k", 1<<10},
		{"K", 1<<10},
		{"M", 1<<20},
		{"G", 1<<30},
		{NULL, 0}
	};

	ret = strtoll(s,&p,0);

	if (p && !*p) return ret;

	for (i=0; multipliers[i].s; i++) {
		if (strcmp(p, multipliers[i].s)==0) {
			ret *= multipliers[i].m;
			return ret;
		}
	}

	fprintf(stderr,"Unknown multiplier %s\n", p);
	exit(1);
	return ret;
}

static void swap_bytes(char *p, int n)
{
	char c;
	while (n) {
		c = p[0];
		p[0] = p[1];
		p[1] = c;
		n -= 2;
		p += 2;
	}
}

static int all_zero(const char *buf, size_t count)
{
	while (count--) {
		if (*buf != 0) return 0;
	}
	return 1;
}

/* varient of write() that will produce a sparse file if possible */
static ssize_t write_sparse(int fd, const void *buf, size_t count)
{
	loff_t ofs;
	if (!sparse_writes || !all_zero(buf, count)) {
		return write(fd, buf, count);
	}
	ofs = llseek(fd, 0, SEEK_CUR) + count;
	if (ftruncate64(fd, ofs) != 0) {
		return -1;
	}
	llseek(fd, count, SEEK_CUR);
	return count;
}

static void usage(void)
{
	printf("tdd has the same options as dd plus some additional options:\n");
	printf("-v      increase verbosity\n");
}


int main(int argc, char *argv[])
{
	char *inf=NULL, *outf=NULL;
	loff_t count=-1, iseek=0, skip=0, total_in=0, total_out=0, n=0;
	loff_t bytes_in=0, bytes_out=0;
	int bswap = 0;
	int fd1=0, fd2=1, i;
	char *buf;
	int ibs = 512;
	int obs = 512;
	int ret = -1;
	extern char *optarg;
	extern int optind;
	int c;
	int verbose = 0;

	while ((c = getopt(argc, argv, "vh")) != -1 ){
		switch (c) {
		case 'v':
			verbose++;
			break;
			
		case 'h':
		default:
			usage();
			exit(1);
		}
	}

	for (i=optind;i<argc;i++) {
		char *a = argv[i];
		if (strncmp(a, "bs=", 3) == 0) {
			ibs = obs = getval(a+3);
		} else if (strncmp(a,"ibs=", 4) == 0) {
			ibs = getval(a+4);
		} else if (strncmp(a,"obs=", 4) == 0) {
			obs = getval(a+4);
		} else if (strncmp(a,"if=", 3) == 0) {
			inf = a+3;
		} else if (strncmp(a,"of=", 3) == 0) {
			outf = a+3;
		} else if (strncmp(a,"seek=", 5) == 0) {
			iseek = getval(a+5);
		} else if (strncmp(a,"skip=", 4) == 0) {
			skip = getval(a+5);
		} else if (strncmp(a,"count=", 6) == 0) {
			count = getval(a+6);
		} else if (strncmp(a,"conv=", 4) == 0) {
			if (strcmp(a+5, "sparse") == 0) {
				sparse_writes=1;
			} else if (strcmp(a+5, "swab") == 0) {
				bswap = 1;
			} else {
				fprintf(stderr,"%s not supported\n", a);
				exit(1);
			}
		} else {
			fprintf(stderr,"unknown option %s\n", a);
			exit(1);
		}
	}

	if (bswap && (ibs & 1)) {
		fprintf(stderr,"ibs must be even for swab\n");
		exit(1);
	}

	buf = (char *)malloc(ibs==obs?ibs:ibs+obs);
	if (!buf) {
		fprintf(stderr,"failed to allocate buffer\n");
		exit(1);
	}

	if (inf) fd1 = open(inf,O_RDONLY|O_LARGEFILE);
	if (fd1 == -1) {
		perror(inf);
		exit(1);
	}

	if (outf) fd2 = open(outf,O_WRONLY|O_CREAT|O_TRUNC|O_LARGEFILE,0666);
	if (fd2 == -1) {
		perror(outf);
		exit(1);
	}

	if (iseek) {
		if (llseek(fd2, iseek*obs, SEEK_CUR) != iseek*obs) {
			fprintf(stderr,"failed to seek to %lld\n", iseek*obs);
			exit(1);
		}
	}

	while (skip) {
		loff_t n = read(fd1, buf, ibs);
		if (n <= 0) {
			fprintf(stderr,"read on input gave %lld\n", n);
			exit(1);
		}
		skip--;
	}
	
	while (count == -1 || count != total_in) {
		loff_t nread;

		nread = read(fd1, buf+n, ibs);
		if (nread == -1) {
			fprintf(stderr,"read failure");
			goto out1;
		}
		if (nread == 0) goto out1;

		total_in++;
		bytes_in += nread;

		if (bswap) {
			swap_bytes(buf+n, nread);
		}

		n += nread;

		while (n >= obs) {
			loff_t nwritten = write_sparse(fd2, buf, obs);
			if (nwritten > 0) bytes_out += nwritten;
			if (nwritten != obs) {
				fprintf(stderr,"write failure\n");
				goto out2;
			}
			n -= nwritten;
			if (n > 0) {
				memmove(buf, buf+obs, n);
			}
			total_out++;
			if (verbose) {
				printf("%lld M\r", bytes_out/(1<<20));
				fflush(stdout);
			}
		}		
	}

	ret = 0;

 out1:
	if (n) {
		loff_t nwritten = write_sparse(fd2, buf, n);
		if (nwritten > 0) bytes_out += nwritten;
	}

 out2:
	close(fd1);
	close(fd2);

	printf("%lld+%d records in\n", bytes_in/ibs, bytes_in%ibs?1:0);
	printf("%lld+%d records out\n", bytes_out/obs, bytes_out%obs?1:0);
	
	return ret;
}
