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
rline version 1.0
Copyright 2002 Andrew Tridgell <tridge@samba.org> 
Released under the GNU General Public License v2 or later

Usage:
  rline COMMAND

This add readline support to any command line driven program. 

rline will try to load a completions file from one of the following locations
in this order:
     $RLINE_COMPLETIONS_FILE
     $RLINE_COMPLETIONS_DIR/COMMAND
     $HOME/.rline/COMMAND
     /usr/share/rline/COMMAND

A completion file consists of one completion per line, with multi-part completions
separated by spaces. You can use the special word [FILE] to mean filename completion.
");
}

/* list of command completions */
static char **cmd_list;
static int num_commands;
static int cmd_offset;

/*
  load completions from the specified filename
*/
static int load_completions_file(const char *fname)
{
	FILE *f;
	char line[200];

	f = fopen(fname, "r");
	if (!f) {
		return 0;
	}

	/* don't bother parsing multi-part completions here, that is done at runtime */
	while (fgets(line, sizeof(line), f)) {
		int len = strlen(line);
		if (len == 0) continue;
		line[len-1] = 0;
		cmd_list = (char **)realloc(cmd_list, sizeof(char *)*(num_commands+1));
		if (!cmd_list) break;
		cmd_list[num_commands++] = strdup(line);
	}

	fclose(f);
	return 1;
}

/* try loading a completions list from varios places */
static void load_completions(const char *command)
{
	char *fname;
	char *p;

	/* take only the last part of the command */
	if ((p = strrchr(command, '/'))) {
		command = p+1;
	}

	/* try $RLINE_COMPLETIONS_FILE */
	if ((p = getenv("RLINE_COMPLETIONS_FILE"))) {
		if (load_completions_file(p)) return;
	}

	/* try $RLINE_COMPLETIONS_DIR */
	if ((p = getenv("RLINE_COMPLETIONS_DIR"))) {
		asprintf(&fname, "%s/%s", p, command);
		if (load_completions_file(fname)) {
			free(fname);
			return;
		}
		free(fname);
	}

	/* try $HOME/.rline/ */
	if ((p=getenv("HOME"))) {
		asprintf(&fname, "%s/.rline/%s", p, command);
		if (load_completions_file(fname)) {
			free(fname);
			return;
		}
		free(fname);
	}

	/* try /usr/share/rline/ */
	asprintf(&fname, "/usr/share/rline/%s", command);
	if (load_completions_file(fname)) {
		free(fname);
		return;
	}
	free(fname);

}


/*
  command completion generator, including multi-part completions
 */
static char *command_generator(const char *line, int state)
{
	static int idx, len;

	if (!state) {
		/* first call */
		idx = 0;
		len = strlen(line);
	}

	/* find the next command that matches both the line so far and
	   the next part of the command */
	for (;idx<num_commands;idx++) {
		if (strncmp(rl_line_buffer, cmd_list[idx], cmd_offset) == 0) {
			if (strcmp(cmd_list[idx] + cmd_offset, "[FILE]") == 0) {
				/* we want filename completion for this one. This must
				   be the last completion */
				rl_filename_completion_desired = 1;
				rl_filename_quoting_desired = 1;
				idx = num_commands;
				return NULL;
			}
			if (strncmp(line, cmd_list[idx] + cmd_offset, len) == 0) {
				char *p = cmd_list[idx++] + cmd_offset;
				/* return only the current part */
				return strndup(p, strcspn(p, " "));
			}
		}
	}

	/* we don't want the filename completer */
	rl_attempted_completion_over = 1;

	return NULL;
}


/* 
   our completion function, just an interface to our command generator
 */
static char **completion_function(const char *line, int start, int end)
{
	cmd_offset = start;

	return (char **)completion_matches(line, command_generator);
}

/* used by line_handler */
static int pipe_fd;

/* callback function when readline has a whole line */
void line_handler(char *line) 
{
	if (!line) exit(0);
	/* send the line down the pipe to the command */
	dprintf(pipe_fd, "%s\n", line);
	if (*line) {
		/* only add non-empty lines to the history */
		add_history(line);
	}
}

/*
  main program
*/
int main(int argc, char *argv[])
{
	int fd1[2], fd2[2];
	char *prompt;

	if (argc < 2) {
		usage();
		exit(1);
	}

	/* load the completions list */
	load_completions(argv[1]);

	/* we will use pipes to talk to the child */
	if (pipe(fd1) != 0 || pipe(fd2) != 0) {
		perror("pipe");
		exit(1);
	}

	/* start the child process */
	if (fork() == 0) {
		close(fd1[1]);
		close(fd2[0]);
		close(0);
		close(1);
		dup2(fd1[0], 0);
		dup2(fd2[1], 1);
		execvp(argv[1], argv+1);
		/* it failed?? maybe command not found */
		perror(argv[1]);
		exit(1);
	}

	/* remember the connection to the child */
	pipe_fd = fd1[1];

	close(fd2[1]);
	close(fd1[0]);

	/* initial blank prompt */
	prompt = strdup("");

	/* install completion handling */
	rl_already_prompted = 1;
	rl_attempted_completion_function = completion_function;
	rl_callback_handler_install(prompt, line_handler);

	/* main loop ... */
	while (1) {
		fd_set fds;
		int ret;

		FD_ZERO(&fds);
		FD_SET(0, &fds);
		FD_SET(fd2[0], &fds);

		/* wait for some activity */
		ret = select(fd2[0]+1, &fds, NULL, NULL, NULL);
		if (ret == -1 && errno == EINTR) continue;
		if (ret <= 0) break;

		/* give any stdin to readline */
		if (FD_ISSET(0, &fds)) {
			rl_callback_read_char();
		}

		/* data from the program is used to intuit the
		   prompt. This works surprisingly well */
		if (FD_ISSET(fd2[0], &fds)) {
			char buf[1024];
			char *p;
			int n = read(fd2[0], buf, sizeof(buf)-1);
			if (n <= 0) break;
			buf[n] = 0;

			/* send to standard output */
			write(1, buf, n);

			/* work out the next prompt */
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

	/* cleanup the terminal */
	rl_callback_handler_remove();

	return 0;
}
