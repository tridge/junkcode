#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <emcast/libemcast.h>


static int check_channel(int c)
{
	char *url;
	Emcast *emcast;
	int fd, ret;
	fd_set rfds;
	struct timeval tv;

	asprintf(&url,"239.193.0.%d:8208", c);

	ret = emcast_new(&emcast, url);

	fd = emcast_join(emcast, url);

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);

	tv.tv_sec = 1;
	tv.tv_usec = 0;

	ret = select(fd+1, &rfds, NULL, NULL, &tv);

	emcast_leave(emcast);

	if (ret == 1) {
		return 1;
	}


	free(url);

	return 0;
}

int main(void)
{
	int i;

	for (i=0;i<255;i++) {
		printf("checking %d\r", i);
		fflush(stdout);
		if (check_channel(i) || 
		    check_channel(i) ||
		    check_channel(i)) {
			printf("Active channel %d\n", i);
		}
	}

	return 0;
}
