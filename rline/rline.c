/* add readline support to any command
   released under the GNU General Public License version 2 or later
   tridge@samba.org, June 2002
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>

static void usage(void)
{
	printf("
readline [options] <command>

This add readline support to any command line driven program. 

Options:
   -c FILE          load a list of command completions from FILE
   -h               show this help
   --               stop processing options (used to allow options to programs)
");
}

/* list of command completions */
static char **cmd_list;
static int num_commands;

static void load_completions(const char *fname)
{
	FILE *f = fopen(fname, "r");
	char line[200];

	if (!f) {
		perror(fname);
		return;
	}

	while (fgets(line, sizeof(line), f)) {
		int len = strlen(line);
		if (len == 0) continue;
		line[len-1] = 0;
		cmd_list = (char **)realloc(cmd_list, sizeof(char *)*(num_commands+1));
		if (!cmd_list) break;
		cmd_list[num_commands++] = strdup(line);
	}

	fclose(f);
}


/*
  command completion generator
 */
static char *command_generator(const char *line, int state)
{
	static int idx, len;

	if (!cmd_list) return NULL;

	if (!state) {
		/* first call */
		idx = 0;
		len = strlen(line);
	}

	while (idx < num_commands &&
	       strncmp(line, cmd_list[idx], len) != 0) {
		idx++;
	}

	if (idx == num_commands) return NULL;

	return strdup(cmd_list[idx++]);
}


/* 
   our completion function, so we can support tab completion
 */
static char **completion_function(const char *line, int start, int end)
{
	if (start != 0) {
		/* they are trying to complete an argument */
		return NULL;
	}

	return (char **)completion_matches(line, command_generator);
}

static int pipe_fd;

/* callback function when readline has a whole line */
void line_handler(char *line) 
{
	if (!line) exit(0);
	dprintf(pipe_fd, "%s\n", line);
	if (*line) {
		add_history(line);
	}
}

int main(int argc, char *argv[])
{
	int fd1[2], fd2[2];
	char *prompt;
	int c;
	extern int optind;
	extern char *optarg;

	while ((c = getopt(argc, argv, "c:h")) != -1) {
		switch (c) {
		case '-':
			break;
		case 'c':
			load_completions(optarg);
			break;
		case 'h':
		default:
			usage();
			exit(1);
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1) {
		usage();
		exit(1);
	}

	if (pipe(fd1) != 0 || pipe(fd2) != 0) {
		perror("pipe");
		exit(1);
	}

	if (fork() == 0) {
		close(fd1[1]);
		close(fd2[0]);
		close(0);
		close(1);
		dup2(fd1[0], 0);
		dup2(fd2[1], 1);
		return execvp(argv[0], argv);
	}

	pipe_fd = fd1[1];

	close(fd2[1]);
	close(fd1[0]);

	prompt = strdup("");
	rl_already_prompted = 1;
	rl_attempted_completion_function = completion_function;
	rl_callback_handler_install(prompt, line_handler);

	while (1) {
		fd_set fds;
		int ret;

		FD_ZERO(&fds);
		FD_SET(0, &fds);
		FD_SET(fd2[0], &fds);

		ret = select(fd2[0]+1, &fds, NULL, NULL, NULL);
		if (ret == -1 && errno == EINTR) continue;
		if (ret <= 0) break;

		if (FD_ISSET(0, &fds)) {
			rl_callback_read_char();
		}

		if (FD_ISSET(fd2[0], &fds)) {
			char buf[1024];
			char *p;
			int n = read(fd2[0], buf, sizeof(buf)-1);
			if (n <= 0) break;
			buf[n] = 0;
			write(1, buf, n);

			p = strrchr(buf, '\n');
			if (!p) {
				p = buf;
			} else {
				p++;
			}

			free(prompt);
			prompt = strdup(p);

			rl_set_prompt(prompt);
			rl_on_new_line_with_prompt();
		}
	}

	rl_callback_handler_remove();

	return 0;
}
