/* add readline support to any command
   Copyright 2002 Andrew Tridgell <tridge@samba.org>, June 2002

   released under the GNU General Public License version 2 or later
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pty.h>
#include <fcntl.h>
#include <utmp.h>
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
		if (*line) {
			cmd_list[num_commands++] = strdup(line);
		}
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
static int child_fd;

/* callback function when readline has a whole line */
void line_handler(char *line) 
{
	if (!line) exit(0);
	/* send the line down the pipe to the command */
	dprintf(child_fd, "%s\n", line);
	if (*line) {
		/* only add non-empty lines to the history */
		add_history(line);
	}
}


/* mirror echo mode from a slave terminal to our terminal */
static void mirror_echo_mode(int fd)
{  
	struct termios pterm1, pterm2;
	
	if (tcgetattr(fd, &pterm1) != 0) return;
	if (tcgetattr(0, &pterm2) != 0) return;

	pterm2.c_lflag &= ~ICANON;
	pterm2.c_lflag &= ~ECHO;
	pterm2.c_lflag |= ISIG;
	pterm2.c_cc[VMIN] = 1;
	pterm2.c_cc[VTIME]=0;
	pterm2.c_lflag &= ~(ICANON|ISIG|ECHO|ECHONL|ECHOCTL| 
			    ECHOE|ECHOK|ECHOKE| 
			    ECHOPRT);
	if (pterm1.c_lflag) {
		pterm2.c_lflag |= ECHO;
	} else {
		pterm2.c_lflag &= ~ECHO;
	}
	tcsetattr(0, TCSANOW, &pterm2);
}


/* setup the slave side of a pty appropriately */
static void setup_terminal(int fd)
{
	struct termios term;

	/* fetch the old settings */
	if (tcgetattr(fd, &term) != 0) return;

	term.c_cc[VMIN] = 1;
	term.c_cc[VTIME] = 0;
	/* we don't want things like echo or other processing */
	term.c_iflag |= IGNBRK;
	term.c_lflag &= ~(ICANON|ISIG|ECHO|ECHONL|ECHOCTL| 
			  ECHOE|ECHOK|ECHOKE| 
			  ECHOPRT);
	tcsetattr(fd, TCSANOW, &term);
}

/*
  main program
*/
int main(int argc, char *argv[])
{
	char *prompt;
	pid_t pid;
	struct termios term, *pterm=NULL;
	int slave_fd;
	char slave_name[100];

	if (argc < 2 || argv[1][0] == '-') {
		usage();
		exit(1);
	}

	/* load the completions list */
	load_completions(argv[1]);

	if (tcgetattr(0, &term) == 0) {
		pterm = &term;
	}

	/* by using forkpty we give a true pty to the child, which means it should 
	   behave the same as if run from a terminal */
	pid = forkpty(&child_fd, slave_name, pterm, NULL);

	if (pid == (pid_t)-1) {
		perror("forkpty");
		exit(1);
	}

	/* the child just sets up its pty then executes the command */
	if (pid == 0) {
		setup_terminal(0);
		execvp(argv[1], argv+1);
		/* it failed?? maybe command not found */
		perror(argv[1]);
		exit(1);
	}

	slave_fd = open(slave_name, O_RDWR);

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
		FD_SET(child_fd, &fds);

		/* wait for some activity */
		ret = select(child_fd+1, &fds, NULL, NULL, NULL);
		if (ret == -1 && errno == EINTR) continue;
		if (ret <= 0) break;

		/* give any stdin to readline */
		if (FD_ISSET(0, &fds)) {
			rl_callback_read_char();
		}

		/* data from the program is used to intuit the
		   prompt. This works surprisingly well */
		if (FD_ISSET(child_fd, &fds)) {
			char buf[1024];
			char *p;
			int n = read(child_fd, buf, sizeof(buf)-1);
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

			/* tell readline about the new prompt */
			free(prompt);
			prompt = strdup(p);

			rl_set_prompt(prompt);
			rl_on_new_line_with_prompt();
			if (slave_fd != -1) {
				mirror_echo_mode(slave_fd);
			}
		}
	}

	/* cleanup the terminal */
	rl_callback_handler_remove();

	return 0;
}
