#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

static int am_sender;

#define rprintf fprintf
#define FERROR stderr
#define exit_cleanup(x) exit(1)
#define do_fork fork
#define STDIN_FILENO 0
#define STDOUT_FILENO 1

#define TOTAL_SIZE (10*1024*1024)

static int write_loop(int fd, char *buf, int size)
{
	int total=0;

	while (size) {
		int n = write(fd, buf, size);
		if (n <= 0) break;
		size -= n;
		buf += n;
		total += n;
	}
	return total;
}

int piped_child(char *command,int *f_in,int *f_out)
{
  int pid;
  int to_child_pipe[2];
  int from_child_pipe[2];

  if (pipe(to_child_pipe) < 0 ||
      pipe(from_child_pipe) < 0) {
    rprintf(FERROR,"pipe: %s\n",strerror(errno));
    exit_cleanup(RERR_IPC);
  }


  pid = do_fork();
  if (pid < 0) {
    rprintf(FERROR,"fork: %s\n",strerror(errno));
    exit_cleanup(RERR_IPC);
  }

  if (pid == 0)
    {
      if (dup2(to_child_pipe[0], STDIN_FILENO) < 0 ||
	  close(to_child_pipe[1]) < 0 ||
	  close(from_child_pipe[0]) < 0 ||
	  dup2(from_child_pipe[1], STDOUT_FILENO) < 0) {
	rprintf(FERROR,"Failed to dup/close : %s\n",strerror(errno));
	exit_cleanup(RERR_IPC);
      }
      if (to_child_pipe[0] != STDIN_FILENO) close(to_child_pipe[0]);
      if (from_child_pipe[1] != STDOUT_FILENO) close(from_child_pipe[1]);
      system(command);
      exit_cleanup(RERR_IPC);
    }

  if (close(from_child_pipe[1]) < 0 ||
      close(to_child_pipe[0]) < 0) {
    rprintf(FERROR,"Failed to close : %s\n",strerror(errno));   
    exit_cleanup(RERR_IPC);
  }

  *f_in = from_child_pipe[0];
  *f_out = to_child_pipe[1];

  return pid;
}

static void sender(int fin, int fout)
{
	int n;
	char buf[1024];
	int total = 0;

	while (total < TOTAL_SIZE) {
		n = read(fin, buf, sizeof(buf));
		if (n <= 0) {
			fprintf(stderr,"write error in sender at %d\n", total);
			break;
		}
		write_loop(fout, buf, n);
		total += n;
		fprintf(stderr, "-");
	}
	fprintf(stderr, "sender done\n");
}

static void generator(int fd)
{
	int n;
	char buf[1024];
	int total=0;

	while (total < TOTAL_SIZE) {
		n = 1 + random() % (sizeof(buf)-1);
		n = write_loop(fd, buf, n);
		if (n <= 0) {
			fprintf(stderr,"write error in generator at %d\n", total);
			break;
		}
		total += n;
		fprintf(stderr, "*");
	}
	fprintf(stderr, "generator done\n");
}

static void receiver(int fd)
{
	ssize_t n;
	ssize_t total=0;
	char buf[1024];

	while (total < TOTAL_SIZE) {
		n = read(fd, buf, sizeof(buf));
		if (n <= 0) {
			fprintf(stderr,"read error in receiver\n");
			break;
		}
		total += n;
		fprintf(stderr, "+");
	}
	fprintf(stderr, "receiver done\n");
}

int main(int argc, char *argv[])
{
	int c, f_in, f_out;

	while ((c = getopt(argc, argv, "s")) != -1) {
		switch (c){
		case 's':
			am_sender = 1;
			break;
		}
	}
	
	if (am_sender) {
		sender(0, 1);
	} else {
		char *command = argv[1];
		printf("running %s\n", command);
		piped_child(command, &f_in, &f_out);
		if (fork()) {
			generator(f_out);
		} else {
			receiver(f_in);
		}
	}

	return 0;
}
