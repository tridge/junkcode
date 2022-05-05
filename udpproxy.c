#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h>

static double timestamp()
{
    struct timeval tval;
    gettimeofday(&tval,NULL);
    return tval.tv_sec + (tval.tv_usec*1.0e-6);
}

/*
  open a socket of the specified type, port and address for incoming data
*/
int open_socket_in(int port)
{
	struct sockaddr_in sock;
	int res;
	int one=1;

	memset(&sock,0,sizeof(sock));

#ifdef HAVE_SOCK_SIN_LEN
	sock.sin_len = sizeof(sock);
#endif
	sock.sin_port = htons(port);
	sock.sin_family = AF_INET;

	res = socket(AF_INET, SOCK_DGRAM, 0);
	if (res == -1) { 
		fprintf(stderr, "socket failed\n"); return -1; 
		return -1;
	}

	setsockopt(res,SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof(one));

	if (bind(res, (struct sockaddr *)&sock, sizeof(sock)) < 0) { 
		return(-1); 
	}

	return res;
}

static void main_loop(int sock1, int sock2)
{
	unsigned char buf[10240];
        bool have_conn1=false;
        bool have_conn2=false;
        double last_pkt1=0;
        double last_pkt2=0;
        int fdmax = (sock1>sock2?sock1:sock2)+1;

	while (1) {
            fd_set fds;
            int ret;
            struct timeval tval;
            double now = timestamp();
            
            if (have_conn1 && now - last_pkt1 > 10) {
                break;
            }
            if (have_conn2 && now - last_pkt2 > 10) {
                break;
            }
            
            FD_ZERO(&fds);
            FD_SET(sock1, &fds);
            FD_SET(sock2, &fds);

            tval.tv_sec = 10;
            tval.tv_usec = 0;

            ret = select(fdmax, &fds, NULL, NULL, &tval);
            if (ret == -1 && errno == EINTR) continue;
            if (ret <= 0) break;

            now = timestamp();
                
            if (FD_ISSET(sock1, &fds)) {
                struct sockaddr_in from;
                socklen_t fromlen = sizeof(from);
                int n = recvfrom(sock1, buf, sizeof(buf), 0, 
                                 (struct sockaddr *)&from, &fromlen);
                if (n <= 0) break;
                last_pkt1 = now;
                if (!have_conn1) {
                    if (connect(sock1, (struct sockaddr *)&from, fromlen) != 0) {
                        break;
                    }
                    have_conn1 = true;
                    printf("have conn1\n");
                    fflush(stdout);
                }
                if (have_conn2) {
                    if (send(sock2, buf, n, 0) != n) {
                        break;
                    }
                }
            }

            if (FD_ISSET(sock2, &fds)) {
                struct sockaddr_in from;
                socklen_t fromlen = sizeof(from);
                int n = recvfrom(sock2, buf, sizeof(buf), 0, 
                                 (struct sockaddr *)&from, &fromlen);
                if (n <= 0) break;
                last_pkt2 = now;
                if (!have_conn2) {
                    if (connect(sock2, (struct sockaddr *)&from, fromlen) != 0) {
                        break;
                    }
                    have_conn2 = true;
                    printf("have conn2\n");
                    fflush(stdout);
                }
                if (have_conn1) {
                    if (send(sock1, buf, n, 0) != n) {
                        break;
                    }
                }
            }
	}
}

static void loop_proxy(int listen_port1, int listen_port2)
{
    while (true) {
        printf("Opening sockets %d %d\n", listen_port1, listen_port2);
        fflush(stdout);
        int sock_in1 = open_socket_in(listen_port1);
        int sock_in2 = open_socket_in(listen_port2);
        if (sock_in1 == -1 || sock_in2 == -1) {
            printf("sock on ports %d or %d failed - %s\n",
                   listen_port1, listen_port2, strerror(errno));
            fflush(stdout);
            if (sock_in1 != -1) {
                close(sock_in1);
            }
            if (sock_in2 != -1) {
                close(sock_in2);
            }
            sleep(5);
            return;
        }
        
        main_loop(sock_in1, sock_in2);
        close(sock_in1);
        close(sock_in2);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 4) {
        printf("Usage: udpproxy <baseport1> <baseport2> <count>\n");
        exit(1);
    }

    int listen_port1 = atoi(argv[1]);
    int listen_port2 = atoi(argv[2]);
    int count = atoi(argv[3]);

    printf("Opening %d sockets\n", count);
    fflush(stdout);
    for (int i=0; i<count; i++) {
        if (fork() == 0) {
            loop_proxy(listen_port1+i, listen_port2+i);
        }
    }
    int status=0;
    wait(&status);

    return 0;
}
