/* a simple proxy with rsync support - meant as a testbed for rsync
   enabled http 
   tridge@samba.org, June 1999 */

#include "rproxy.h"


static char *upstream;
static int upstream_port=80;

static void log_transfer(int siglen, 
			 int inbytes, int outbytes, 
			 int zinbytes, int zoutbytes, 
			 char *req)
{
	FILE *f = fopen("rproxy.log", "a");
	if (f) {
		fprintf(f,"%4d %6d %6d %6d %6d %s\n", 
			siglen, 
			inbytes, outbytes, 
			zinbytes, zoutbytes, 
			req);
		fclose(f);
	}
}

/* send a requst to the upstream proxy then return a FILE pointer for
   the upstream socket */
static FILE *send_upstream(char *request, char *body, int content_length)
{
	int fd;
	FILE *f;

	fd = open_socket_out(upstream, upstream_port);
	if (fd == -1) {
		printf("Failed to connect to %s:%d\n", 
		       upstream, upstream_port);
		exit_cleanup(1);
	}

	f = xfdopen(fd, "r+");
	setbuffer(f, NULL, BUFFER_SIZE);
	
	/* send the request */
	fprintf(f,"%s\r\n", request);
	
	/* the headers */
	header_send(f);

	/* and the body */
	if (body) {
		fwrite(body, content_length, 1, f);
		printf("sent body of size %d\n", content_length);
	}
	fflush(f);

	return f;
}

/* this is called on a new connection. The fd is the socket */
static int child_main(int fd)
{
	char request[1024];
	char response[1024];
	int content_length = 0;
	FILE *f, *f_in, *f_cache=NULL, *f_cache2;
	char *body = NULL;
	char *request_signature=NULL;
	char *encoding;
	int rsync_encoded;
	extern size_t sig_inbytes, sig_outbytes;
	extern size_t sig_zinbytes, sig_zoutbytes;

	/* stdio is convenient in this case */
	f = xfdopen(fd, "r+");
	setbuffer(f, NULL, BUFFER_SIZE);

	/* read the main request */
	if (!fgets_strip(request, sizeof(request)-1, f)) {
		printf("failed to read request\n");
		exit_cleanup(1);
	}

	printf("[R] %s\n", request);
	
	/* read the headers */
	header_load(f);

	/* read the request body based on the content length */
	if (header_content("Content-Length")) {
		body = body_load(f, &content_length);
	}

	/* damn IRIX needs a rewind here ... */
	rewind(f);

	request_signature = header_content("Rsync-Signature");
	if (request_signature) request_signature = xstrdup(request_signature);

	/* now check to see if we have a cached file for this URL, and
	   if we do then generate the signature and add it as a header 

	   if the downstream socket knows about rsync signatures then
	   we don't generate one */
	if (!request_signature && (f_cache = cache_open(request, "r"))) {
		char *sigblock;
		printf("Have cached file\n");
		sigblock = sig_generate(f_cache);
		header_add("Rsync-Signature", sigblock);
		free(sigblock);
	}

	/* send out the request to the upstream proxy */
	f_in = send_upstream(request, body, content_length);

	/* clear the headers */
	header_clear();
	if (body) free(body);

	/* read the response from the upstream */

	/* read the main response code */
	if (!fgets_strip(response, sizeof(response)-1, f_in)) {
		printf("failed to read response\n");
		exit_cleanup(1);
	}

	printf("[R] %s\n", response);
	
	/* read the headers */
	header_load(f_in);

	content_length = header_ival("Content-Length");

	/* 
	   there are now 4 possibilities:

	   1) the downstream socket did supply us with a rsync
	   signature and we got a rsync-encoded reply. We send on the
	   reply as is.

	   2) the downstream socket didn't supply a rsync signature
	   and we didn't get a rsync-encoded reply. just send the
	   reply on unmolested.

	   3) the downstream socket didn't supply a rsync signature
	   but we got an rsync-encoded reply. We need to decode it
	   before sending it on

	   4) the downstream socket did supply us with a rsync
	   signature and we got a non-encoded reply. We need to rsync
	   encode the reply and send it on.

	*/

	encoding = header_content("Content-Encoding");
	rsync_encoded = (encoding && strstr(encoding,"rsync") != NULL);

	printf("encoded=%d siglen=%d\n", 
	       rsync_encoded, request_signature?strlen(request_signature):0);

	if ((request_signature && rsync_encoded) ||
	    (!request_signature && !rsync_encoded)) {
		int stream_size;

		/* case 1 and 2, send as-is */
		printf("sending as-is\n");

		if (f_cache) fclose(f_cache);

		if (!rsync_encoded) {
			/* only cache the reply if it isn't rsync encoded */
			f_cache = cache_open_tmp(request,"w");
		}
	
		fprintf(f,"%s\r\n", response);
		header_send(f);

		/* stream the body across */	  
		stream_size = stream_body(f_in, f, f_cache, content_length);
		fflush(f);	
		
		if (f_cache) {
			fclose(f_cache);
		}

		if (stream_size < MIN_CACHE_SIZE && f_cache) {
			/* don't keep really small files */
			cache_tmp_delete(request);
		} else {
			cache_tmp_rename(request);
		}
		
		goto finish;
	}

	/* both case 3 and 4 should cache the reply */

	if (!request_signature && rsync_encoded) {
		printf("case 3\n");

		/* case 3, we need to decode the rsync-encoded reply
		   then remove the encoding header, adjust the
		   content-length header, cache the result and send it
		   on 

		   hmmm, problem. We don't know the new content-length
		   until after we have decoded the data. For now I'll
		   remove the content-length header rather than lose
		   the streaming */
		f_cache2 = cache_open_tmp(request,"w");

		header_remove("Content-Length");
		header_remove_list("Content-Encoding", "rsync");

		fprintf(f,"%s\r\n", response);
		header_send(f);

		sig_decode(f_in, f, f_cache, f_cache2, content_length);
		cache_tmp_rename(request);
		fclose(f_cache);
		fclose(f_cache2);

		goto finish;
	}

	if (request_signature && !rsync_encoded) {
		printf("case 4\n");

		/* case 4, we need to rsync-encode the reply while
		   cacheing the data, add a encoding header, adjust
		   the content-length header then send it on 

		   we suffer from the same problem here with the
		   content-length value. I'll get rid of it for now. */
		f_cache2 = cache_open_tmp(request,"w");

		header_remove("Content-Length");
		header_add_list("Content-Encoding", "rsync");

		fprintf(f,"%s\r\n", response);
		header_send(f);

		sig_encode(f_in, f, request_signature, f_cache2, 
			   content_length);

		cache_tmp_rename(request);
		fclose(f_cache2);

		/* logging at this point catches the important case */
		log_transfer(strlen(request_signature), 
			     sig_inbytes, sig_outbytes, 
			     sig_zinbytes, sig_zoutbytes, 
			     request);

		goto finish;
	}		

	printf("oops, maybe I can't count?\n");

 finish:
	
	return 0;
}

static void usage(void)
{
	printf("read the code\n");
}

void exit_cleanup(int code)
{
	exit(code);
}

int main(int argc, char *argv[])
{
	static int port = 8080;
	char *p;

	if (argc < 2) {
		usage();
		exit_cleanup(1);
	}

	upstream = argv[1];
	p = strchr(upstream,':');
	if (p) {
		*p = 0;
		upstream_port = atoi(p+1);
	}

	if (argc > 2) {
		port = atoi(argv[2]);
	}

	mkdir(CACHE_DIR,0755);

	start_accept_loop(port, child_main);
	return 0;
}
