/*
  a simple interface to syslog() 
  tridge@samba.org
*/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>

static struct {
	char *name;
	int level;
} facilities[] = {
	{"LOG_AUTH", LOG_AUTH},
	{"LOG_AUTHPRIV", LOG_AUTHPRIV},
	{"LOG_CRON", LOG_CRON},
	{"LOG_DAEMON", LOG_DAEMON},
	{"LOG_KERN", LOG_KERN},
	{"LOG_LOCAL0", LOG_LOCAL0},
	{"LOG_LOCAL1", LOG_LOCAL1},
	{"LOG_LOCAL2", LOG_LOCAL2},
	{"LOG_LOCAL3", LOG_LOCAL3},
	{"LOG_LOCAL4", LOG_LOCAL4},
	{"LOG_LOCAL5", LOG_LOCAL5},
	{"LOG_LOCAL6", LOG_LOCAL6},
	{"LOG_LOCAL7", LOG_LOCAL7},
	{"LOG_LPR", LOG_LPR},
	{"LOG_MAIL", LOG_MAIL},
	{"LOG_NEWS", LOG_NEWS},
	{"LOG_USER", LOG_USER},
	{"LOG_UUCP", LOG_UUCP},
	{NULL, 0}
};

static struct {
	char *name;
	int level;
} priorities[] = {
	{"LOG_EMERG", LOG_EMERG},
	{"LOG_ALERT", LOG_ALERT},
	{"LOG_CRIT", LOG_CRIT},
	{"LOG_ERR", LOG_ERR},
	{"LOG_WARNING", LOG_WARNING},
	{"LOG_NOTICE", LOG_NOTICE},
	{"LOG_INFO", LOG_INFO},
	{"LOG_DEBUG", LOG_DEBUG},
	{NULL, 0}
};

int main(int argc, char *argv[])
{
	char *ident, *facility, *priority, *msg;
	int i_facility, i_priority, i;

	if (argc != 5) {
		fprintf(stderr,"Usage: syslog ident facility priority msg\n");
		exit(1);
	}

	ident = argv[1];
	facility = argv[2];
	priority = argv[3];
	msg = argv[4];

	/* map the facility */
	for (i=0;facilities[i].name;i++) {
		if (strcmp(facilities[i].name, facility) == 0) break;
	}
	if (!facilities[i].name) {
		i_facility = atoi(facility);
	} else {
		i_facility = facilities[i].level;
	}

	/* map the priority */
	for (i=0;priorities[i].name;i++) {
		if (strcmp(priorities[i].name, priority) == 0) break;
	}
	if (!priorities[i].name) {
		i_priority = atoi(priority);
	} else {
		i_priority = priorities[i].level;
	}

	/* just in case we are setuid root ... */
	setuid(0);

	openlog(ident, LOG_CONS, i_facility);
	syslog(i_priority, "%s", msg);
	closelog();
	return 0;
}
