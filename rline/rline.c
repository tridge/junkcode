/* add readline support to any command
   Copyright 2002 Andrew Tridgell <tridge@samba.org>, June 2002

   released under the GNU General Public License version 2 or later
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
readline COMMAND

This add readline support to any command line driven program. 
rline will load a completions file from $HOME/.rline/COMMAND

If you specify a RLINE_DATA environment variable then rline will look
in that directory instead.
");
}

/* list of command completions */
static char **cmd_list;
static int num_commands;
static int cmd_offset;

static void load_completions(const char *command)
{
	char *fname;
	FILE *f;
	char line[200];
	char *p;

	if ((p=getenv("RLINE_DATA"))) {
		asprintf(&fname, "%s/%s", p, command);
	} else {
		asprintf(&fname, "%s/.rline/%s", getenv("HOME"), command);
	}

	f = fopen(fname, "r");
	free(fname);
	if (!f) {
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
	char *p;

	if (!cmd_list) return NULL;

	if (!state) {
		/* first call */
		idx = 0;
		len = strlen(line);
	}

	/* find the next command that matches both the line so far and
	   the next part of the command */
	for (;idx<num_commands;idx++) {
		if (strncmp(rl_line_buffer, cmd_list[idx], cmd_offset) == 0 &&
		    strncmp(line, cmd_list[idx] + cmd_offset, len) == 0) {
			p = cmd_list[idx++] + cmd_offset;
			p = strndup(p, strcspn(p, " "));
			if (strcmp(p, "*") == 0) {
				/* we want filename completion for this one. This must
				   be the last completion */
				free(p);
				idx = num_commands;
				return NULL;
			}
			return p;
		}
	}

	/* we don't want the filename completer */
	rl_attempted_completion_over = 1;

	return NULL;
}


/* 
   our completion function, so we can support tab completion
 */
static char **completion_function(const char *line, int start, int end)
{
	cmd_offset = start;

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

	if (argc < 2) {
		usage();
		exit(1);
	}

	load_completions(argv[1]);

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
		return execvp(argv[1], argv+1);
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
