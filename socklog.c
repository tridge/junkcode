#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <syslog.h>

/*******************************************************************
 return the IP addr of the client as a string 
 ******************************************************************/
char *client_addr(int fd)
{
	struct sockaddr sa;
	struct sockaddr_in *sockin = (struct sockaddr_in *) (&sa);
	int     length = sizeof(sa);
	static char addr_buf[100];
	static int initialised;

	if (initialised) return addr_buf;

	initialised = 1;

	if (getpeername(fd, &sa, &length)) {
		exit(1);
	}
	
	strncpy(addr_buf,
		(char *)inet_ntoa(sockin->sin_addr), sizeof(addr_buf));
	addr_buf[sizeof(addr_buf)-1] = 0;

	return addr_buf;
}


/*******************************************************************
 return the DNS name of the client 
 ******************************************************************/
char *client_name(int fd)
{
	struct sockaddr sa;
	struct sockaddr_in *sockin = (struct sockaddr_in *) (&sa);
	int     length = sizeof(sa);
	static char name_buf[100];
	struct hostent *hp;
	char **p;
	char *def = "UNKNOWN";
	static int initialised;

	if (initialised) return name_buf;

	initialised = 1;

	strcpy(name_buf,def);

	if (getpeername(fd, &sa, &length)) {
		exit(1);
	}

	/* Look up the remote host name. */
	if ((hp = gethostbyaddr((char *) &sockin->sin_addr,
				sizeof(sockin->sin_addr),
				AF_INET))) {
		
		strncpy(name_buf,(char *)hp->h_name,sizeof(name_buf));
		name_buf[sizeof(name_buf)-1] = 0;
	}


	/* do a forward lookup as well to prevent spoofing */
	hp = gethostbyname(name_buf);
	if (!hp) {
		strcpy(name_buf,def);
	} else {
		for (p=hp->h_addr_list;*p;p++) {
			if (memcmp(*p, &sockin->sin_addr, hp->h_length) == 0) {
				break;
			}
		}
		if (!*p) {
			strcpy(name_buf,def);
		} 
	}

	return name_buf;
}


int main(int argc, char *argv[])
{
	char *service = argv[1];

	openlog("socklog", LOG_PID, LOG_DAEMON);
	syslog(LOG_ERR, "%s connection from %s [%s]", 
	       service, client_name(0), client_addr(0));
	exit(0);
}
