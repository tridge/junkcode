#include "includes.h"

static struct cgi_state *cgi;

/* this is a helper function for file upload. The scripts can call 
  @save_file(cgi_variablename, filename) to save the contents of 
  an uploaded file to disk
*/
static void save_file(struct template_state *tmpl, 
		      const char *name, const char *value,
		      int argc, char **argv)
{
	char *var_name, *file_name;
	int fd;
	const char *content;
	unsigned size, ret;

	if (argc != 2) {
		printf("Invalid arguments to function %s (%d)\n", name, argc);
		return;
	}
	var_name = argv[0];
	file_name = argv[1];

	content = cgi->get_content(cgi, var_name, &size);
	if (!content) {
		printf("No content for variable %s?\n", var_name);
		return;
	}
	fd = open(file_name, O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (fd == -1) {
		printf("Failed to open %s (%s)\n", file_name, strerror(errno));
		return;
	}
	ret = write(fd, content, size);
	if (ret != size) {
		printf("out of space writing %s (wrote %u)\n", file_name, ret);
	}
	close(fd);
}

/* the main webserver process, called with stdin and stdout setup
 */
static void run_webserver(void)
{
	struct stat st;

	if (chdir("html") != 0) {
		fprintf(stderr,"Can't find html directory?\n");
		exit(1);
	}

	cgi = cgi_init();
	cgi->setup(cgi);
	cgi->load_variables(cgi);
	cgi->tmpl->put(cgi->tmpl, "save_file", "", save_file);

	/* handle a direct file download */
	if (!strstr(cgi->pathinfo, "..") && *cgi->pathinfo != '/' &&
	    stat(cgi->pathinfo, &st) == 0 && S_ISREG(st.st_mode)) {
		cgi->download(cgi, cgi->pathinfo);
		cgi->destroy(cgi);
		return;
	}

	cgi->download(cgi, "index.html");
	cgi->destroy(cgi);
}


/* main program, just start listening and answering queries */
int main(int argc, char *argv[])
{
	int port = TSERVER_PORT;
	extern char *optarg;
	int opt;

	while ((opt=getopt(argc, argv, "p:")) != -1) {
		switch (opt) {
		case 'p':
			port = atoi(optarg);
			break;
		}
	}	

	tcp_listener(port, run_webserver);
	return 0;
}

