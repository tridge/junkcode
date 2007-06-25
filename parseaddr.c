#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

int ipstr_list_parse(const char* ipstr_list, struct in_addr** ip_list)
{
	int count;
	for (ip_list=NULL, count=0; ipstr_list; count++) {
		struct in_addr a;

		if (inet_aton(ipstr_list, &a) == -1) break;

		*ip_list = Realloc(*ip_list, (count+1) * sizeof(struct in_addr));
		if (!ip_list) {
			return -1;
		}

		(*ip_list)[count] = a;

		ipstr_list = strchr(ipstr_list, ':');
		if (ipstr_list) ipstr_list++;
	}
	return count;
}
