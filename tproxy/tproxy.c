#include "includes.h"

static struct cgi_state *cgi;
static char *logfile;

static int write_all(int fd, char *buf, int len)
{
	int len0 = len;

	while (len) {
		int n = write(fd, buf, len);
		if (n == -1) return -1;
		buf += n;
		len -= n;
	}

	return len0;
}

static void set_nonblocking(int fd)
{
	unsigned v = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, v | O_NONBLOCK);
}

/* handle a single request */
static void run_request(int fd1, int fd2, FILE *log_in, FILE *log_out)
{
	struct pollfd polls[2];
	char buf[1024];
	int len;

	polls[0].fd = fd1;
	polls[0].events = POLLIN;
	polls[1].fd = fd2;
	polls[1].events = POLLIN;

	set_nonblocking(fd1);
	set_nonblocking(fd2);

	while (1) {
		int ret = poll(polls, 2, 30000);

		if (ret == -1) {
			perror("poll");
			goto done;
		}

		if (ret == 0) {
			fprintf(stderr,"Timed out\n");
			goto done;
		}
		
		if (polls[0].revents & POLLIN) {
			len = read(polls[0].fd, buf, sizeof(buf));
			if (len <= 0) break;
			write_all(polls[1].fd, buf, len);
			if (log_in) fwrite(buf, 1, len, log_in);
		}

		if (polls[1].revents & POLLIN) {
			len = read(polls[1].fd, buf, sizeof(buf));
			if (len <= 0) break;
			write_all(polls[0].fd, buf, len);
			if (log_out) fwrite(buf, 1, len, log_out);
		}
	}

done:
	if (polls[0].fd != -1) close(polls[0].fd);
	if (polls[1].fd != -1) close(polls[1].fd);
}


/* the main proxy process, called with stdin and stdout setup
 */
static void run_proxy(int count)
{
	char *url;
	char *hostname, *port_str, *request;
	char *s;
	int i, fd, port = 80;
	FILE *log_in = NULL, *log_out = NULL;

	cgi = cgi_init();

	setvbuf(stdin, NULL, _IONBF, 0);

	cgi->setup(cgi);

	fprintf(stderr,"Got URL [%s]\n", cgi->url);

	if (strncasecmp(cgi->url, "http://", 7) != 0) {
		cgi->http_error(cgi, "400 Bad Request", "", 
				"This proxy only supports HTTP");
		exit(1);
	}

	url = cgi->url + 7;
	hostname = strndup(url, strcspn(url, "/"));
	port_str = strchr(hostname, ':');
	request = strchr(url, '/');
	if (!request) request = "/";

	if (port_str) {
		*port_str++ = 0;
		port = atoi(port_str);
	}

	fd = open_socket_out(hostname, port);
	if (fd == -1) {
		cgi->http_error(cgi, "503 Service unavailable", "", 
				"Can't connect to the specified server");
		exit(1);
	}

	if (logfile) {
		asprintf(&s, "%s.%d.in", logfile, count);
		log_in = fopen(s, "w");
		free(s);
		asprintf(&s, "%s.%d.out", logfile, count);
		log_out = fopen(s, "w");
		free(s);
	}

	asprintf(&s,"%s %s%s%s HTTP/1.0\r\n", 
		 cgi->request_post?"POST":"GET", 
		 request,
		 cgi->query_string?"?":"",
		 cgi->query_string?cgi->query_string:"");
	write(fd, s, strlen(s));
	if (log_out) fprintf(log_out, "%s", s);
	free(s);

	for (i=0; i<cgi->num_headers; i++) {
		write(fd, cgi->extra_headers[i], strlen(cgi->extra_headers[i]));
		write(fd, "\r\n", 2);
		if (log_out) fprintf(log_out, "%s\r\n", cgi->extra_headers[i]);
	}
	write(fd, "\r\n", 2);

	run_request(0, fd, log_out, log_in);

	if (log_in) fclose(log_in);
	if (log_out) fclose(log_out);

	cgi->destroy(cgi);
}


/* main program, just start listening and answering queries */
int main(int argc, char *argv[])
{
	int port = TPROXY_PORT;
	extern char *optarg;
	int opt;

	while ((opt=getopt(argc, argv, "p:l:")) != -1) {
		switch (opt) {
		case 'p':
			port = atoi(optarg);
			break;
		case 'l':
			logfile = optarg;
			break;
		}
	}	

	tcp_listener(port, run_proxy);
	return 0;
}

