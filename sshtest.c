/*
  tridge@linuxcare.com January 2000

  This is a sample program that demonstrates a deadlock in sshd when
  handling large amounts of bi-directional data. This deadlock has
  been the cause of lots of problems with rsync over ssh. Although the
  problem only maniifests itself occasionally it makes large rsync
  over ssh transfers unreliable.

  Luckily the fix is very easy - tell sshd to use socketpair() instead
  of pipe(). Just remove the line:
	#define USE_PIPES 1
  from near the bottom of includes.h in the ssh sources.

  Note that fixing the problem by playing with the write sizes or by
  using non-blocking writes due to the following bugs in various OSes.

  - in Linux 2.2 a write to a non-blocking pipe will either return
  EAGAIN if no space is avaiilable or will block if not. This is fixed
  in Linux 2.3.

  - Under IRIX and OSF1 a select on a blocking pipe may return before
  PIPE_BUF can be written. When a subsequent write of more than 1 byte
  is done the write blocks.

  - under some versions of OpenBSD a select on a non-blocking pipe
  will always return immediately, even when no space is available.

  The sum of all these factors means it is not possible to write
  portable code that uses pipes in a way that won't allow deadlocks,
  unless writes are restricted to a single byte after a select on a
  blocking pipe. The performance of that solution would be terrible.

running this code:

  1) "make nettest"
  2) nettest "ssh localhost nettest -s"
  3) if it blocks then you have hit the deadlock.

*/


#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <stdlib.h>

#define TOTAL_SIZE (10*1024*1024)


static int fd_pair(int fd[2])
{
	return socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
}


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

static int piped_child(char *command,int *f_in,int *f_out)
{
	int pid;
	int to_child_pipe[2];
	int from_child_pipe[2];

	if (fd_pair(to_child_pipe) < 0 ||
	    fd_pair(from_child_pipe) < 0) {
		fprintf(stderr,"fd_pair: %s\n",strerror(errno));
		exit(1);
	}


	pid = fork();
	if (pid < 0) {
		fprintf(stderr,"fork: %s\n",strerror(errno));
		exit(1);
	}

	if (pid == 0) {
		if (dup2(to_child_pipe[0], STDIN_FILENO) < 0 ||
		    close(to_child_pipe[1]) < 0 ||
		    close(from_child_pipe[0]) < 0 ||
		    dup2(from_child_pipe[1], STDOUT_FILENO) < 0) {
			fprintf(stderr,"Failed to dup/close : %s\n",strerror(errno));
			exit(1);
		}
		if (to_child_pipe[0] != STDIN_FILENO) close(to_child_pipe[0]);
		if (from_child_pipe[1] != STDOUT_FILENO) close(from_child_pipe[1]);
		system(command);
		exit(1);
	}

	if (close(from_child_pipe[1]) < 0 ||
	    close(to_child_pipe[0]) < 0) {
		fprintf(stderr,"Failed to close : %s\n",strerror(errno));   
		exit(1);
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
	int am_sender = 0;

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
