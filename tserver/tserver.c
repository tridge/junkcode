#include "includes.h"

static void run_webserver(void)
{
	char *download_url, *action;

	if (chdir("files") != 0) {
		fprintf(stderr,"Can't find files directory?\n");
		exit(1);
	}

	cgi_setup();

	download_url = cgi_variable("download_url");
	action = cgi_variable("action");

	cgi_download("header.html");

	printf("<p>Got action %s\n", action);
	printf("<p>Installing from %s\n", download_url);

	dump_file("footer.html");
}

int main(int argc, char *argv[])
{
	tcp_listener(TSERVER_PORT, TSERVER_LOGFILE, run_webserver);
	return 0;
}
