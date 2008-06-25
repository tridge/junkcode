#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

static struct timeval tp1,tp2;

static void start_timer()
{
	gettimeofday(&tp1,NULL);
}

static double end_timer()
{
	gettimeofday(&tp2,NULL);
	return (tp2.tv_sec + (tp2.tv_usec*1.0e-6)) - 
		(tp1.tv_sec + (tp1.tv_usec*1.0e-6));
}


static pid_t piped_child(const char *command[], int *f_in, int *f_out)
{
	pid_t pid;
	int to_child_pipe[2];
	int from_child_pipe[2];

	if (pipe(to_child_pipe) < 0 || pipe(from_child_pipe) < 0) {
		perror("pipe");
		exit(1);
	}

	pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(1);
	}

	if (pid == 0) {
		if (dup2(to_child_pipe[0], STDIN_FILENO) < 0 ||
		    close(to_child_pipe[1]) < 0 ||
		    close(from_child_pipe[0]) < 0 ||
		    dup2(from_child_pipe[1], STDOUT_FILENO) < 0) {
			printf("Failed to dup/close");
			exit(1);
		}
		if (to_child_pipe[0] != 0)
			close(to_child_pipe[0]);
		if (from_child_pipe[1] != 1)
			close(from_child_pipe[1]);
		execvp(command[0], command);
		printf("Failed to exec %s", command[0]);
		exit(1);
	}

	if (close(from_child_pipe[1]) < 0 || close(to_child_pipe[0]) < 0) {
		printf("Failed to close");
		exit(1);
	}

	*f_in = from_child_pipe[0];
	*f_out = to_child_pipe[1];

	return pid;
}


int main(int argc, const char *argv[])
{
	pid_t pid;
	int fd_in, fd_out;
	double max_latency=0.0, min_latency=0.0, total_latency=0.0;
	unsigned num_calls = 0;
	char buf[] = "testing0\n";
	char buf2[10];
	int len = strlen(buf);
	int print_count=0;

	pid = piped_child(argv+1, &fd_in, &fd_out);

	/* ignore first round trip */
	if (write(fd_out, buf, len) != len) {
		printf("write error\n");
		exit(1);
	}
	if (read(fd_in, buf2, len) != len) {
		printf("read error\n");
		exit(1);
	}

	while (1) {
		double lat;
		buf[len-2] = num_calls%10;
		start_timer();
		if (write(fd_out, buf, len) != len) {
			printf("write error\n");
			break;
		}
		if (read(fd_in, buf2, len) != len) {
			printf("read error\n");
			break;
		}
		lat = end_timer();
		if (memcmp(buf, buf2, len) != 0) {
			printf("Wrong data: %s\n", buf2);
		}
		if (lat < min_latency || min_latency == 0.0) {
			min_latency = lat;
		}
		if (lat > max_latency) {
			max_latency = lat;
		}
		total_latency += lat;
		num_calls++;
		if (total_latency > print_count+1) {
			print_count++;
			printf("Calls: %6u Latency: min=%.6f max=%.6f avg=%.6f\n",
			       num_calls, 
			       min_latency, max_latency, total_latency/num_calls);
			min_latency = 0;
			max_latency = 0;
			fflush(stdout);
		}
	}

	return 0;	
}
