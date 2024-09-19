/*
  thermal capture daemon for SIYI ZT30 camera

  client can connect on TCP port 7345 and will get the latest thermal
  image with filename and timestamp
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <dirent.h>
#include <zlib.h>

#define LISTEN_PORT 7345
#define THERMAL_DIR "/mnt/DCIM/102SIYI_TEM"
//#define THERMAL_DIR "/mnt/Photo/B"
//#define THERMAL_DIR "test_images"

#define EXPECTED_SIZE (640 * 512 * 2)

#define FORK_PER_CONNECTION 1

#define PACKED __attribute__((__packed__))

struct PACKED header {
    char fname[128];
    uint32_t compressed_size;
    double timestamp;
};

static int open_socket_in(int port)
{
    struct sockaddr_in sock;
    int res;
    int one=1;

    memset(&sock, 0, sizeof(sock));

    bzero((char *)&sock,sizeof(sock));

    sock.sin_port = htons(port);
    sock.sin_family = AF_INET;

    res = socket(AF_INET, SOCK_STREAM, 0);
    if (res == -1) {
	printf("socket failed\n"); return -1;
    }

    setsockopt(res,SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof(one));

    if (bind(res, (struct sockaddr *)&sock,sizeof(sock)) < 0) {
	return -1;
    }

    return res;
}

static double ts_to_double(struct timespec *ts)
{
    return ((double)((unsigned long)ts->tv_sec)) + ((double)ts->tv_nsec) * 1e-9;
}

static char last_fname[128];

char *find_latest_file(int fd, struct stat *st_latest)
{
    DIR *d = opendir(THERMAL_DIR);
    if (!d) {
	dprintf(fd, "opendir failed\n");
	return NULL;
    }
    char *newest_fname = NULL;
    struct dirent *de;

    while ((de = readdir(d))) {
	char *fname;

	if (strcmp(de->d_name,".")==0) continue;
	if (strcmp(de->d_name,"..")==0) continue;

	asprintf(&fname, "%s/%s", THERMAL_DIR, de->d_name);

	if (newest_fname == NULL ||
	    strcmp(fname, newest_fname) > 0) {
	    if (newest_fname) free(newest_fname);
	    newest_fname = fname;
	} else {
	    free(fname);
	}
    }

    closedir(d);

    if (stat(newest_fname, st_latest) == -1) {
	free(newest_fname);
	return NULL;
    }
    
    return newest_fname;
}

// Function to handle errors
static void zlib_error(const char *msg) {
    perror(msg);
    exit(1);
}

/*
  compress a buffer, returning the compressed size
  exit via zlib_error() on any error
*/
static uint32_t compress_buffer(uint8_t *in_buffer, uint32_t in_size, uint8_t *out_buffer, uint32_t out_buf_size, uint8_t compression_level)
{
    z_stream strm;
    memset(&strm, 0, sizeof(strm)); // Initialize the z_stream structure

    // Initialize the zlib stream for compression
    if (deflateInit(&strm, compression_level) != Z_OK) {
        zlib_error("deflateInit failed");
    }

    // Set up the zlib stream for compression
    strm.avail_in = in_size;
    strm.next_in = in_buffer;

    strm.avail_out = out_buf_size;
    strm.next_out = out_buffer;

    // Compress with zlib
    int ret = deflate(&strm, Z_FINISH);
    if (ret == Z_STREAM_ERROR) {
	deflateEnd(&strm);
	zlib_error("deflate failed");
    }

    // Clean up the zlib stream
    deflateEnd(&strm);
    
    // Calculate how many bytes were compressed into out_buffer
    return out_buf_size - strm.avail_out;
}


static void serve_connection(int fd)
{
    struct stat st;
    char *fname = find_latest_file(fd, &st);
    if (fname == NULL || st.st_size != EXPECTED_SIZE) {
	return;
    }

    if (strcmp(last_fname, fname) == 0) {
	return;
    }
    strncpy(last_fname, fname, sizeof(last_fname));

    struct header h;
    memset(&h, 0, sizeof(h));
    strncpy(h.fname, fname, sizeof(h.fname));
    h.timestamp = ts_to_double(&st.st_mtim);

    int dfd = open(fname, O_RDONLY);
    free(fname);

    if (dfd == -1) {
	return;
    }
    uint8_t buf[EXPECTED_SIZE];
    if (read(dfd, buf, sizeof(buf)) != sizeof(buf)) {
	return;
    }
    uint8_t compressed_buf[EXPECTED_SIZE*2];
    h.compressed_size = compress_buffer(buf, sizeof(buf), compressed_buf, sizeof(compressed_buf), 1);
    if (h.compressed_size == 0) {
	printf("compression failed %s\n", h.fname);
	exit(1);
    }

    write(fd, &h, sizeof(h));
    write(fd, compressed_buf, h.compressed_size);

    printf("Sent %s compression %.1f%% compressed_size=%u\n", h.fname, (100.0 * h.compressed_size) / sizeof(buf), (unsigned)h.compressed_size);
}

static void listener(void)
{
    int sock;

    sock = open_socket_in(LISTEN_PORT);

    if (listen(sock, 200) == -1) {
	fprintf(stderr,"listen failed\n");
	exit(1);
    }

    printf("waiting for connections on port %u\n", (unsigned)LISTEN_PORT);

    while (1) {
	struct sockaddr addr;
	socklen_t in_addrlen = sizeof(addr);
	int fd;

	while (waitpid((pid_t)-1,(int *)NULL, WNOHANG) > 0) ;

	fd = accept(sock, &addr, &in_addrlen);

	if (fd != -1) {
#if FORK_PER_CONNECTION
	    if (fork() == 0) {
		serve_connection(fd);
		exit(0);
	    }
#else
	    serve_connection(fd);
#endif
	    close(fd);
	}
    }
}

int main(void)
{
    listener();
    return 0;
}
