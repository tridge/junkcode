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
readline <command>

This add readline support to any command line driven program. 
");
}

int main(int argc, char *argv[])
{
	int fd1[2], fd2[2];
	char *prompt;

	void line_handler(char *line) {
		if (!line) exit(0);
		dprintf(fd1[1], "%s\n", line);
		if (*line) {
			add_history(line);
		}
	}

	if (argc < 2) {
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
		return execvp(argv[1], argv+1);
	}

	close(fd2[1]);
	close(fd1[0]);

	prompt = strdup("> ");
	rl_already_prompted = 1;
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
