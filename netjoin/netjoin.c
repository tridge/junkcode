/*--

  add a host as a machine account to a ldap server and randomise its
  machine password 

  Written by tridge@samba.org, based on code from AD.tgz from Microsoft
  
--*/

#include "includes.h"

static void usage(void)
{
	printf("Usage: netjoin <ldap_host> <hostname> <realm>\n");
}

int main(int argc, char *argv[])
{
	char *ldap_host;
	char *hostname;
	char *realm;
	char *principal;
	ADS_STRUCT *ads;
	int rc;

	if (argc != 4) {
		usage();
		exit(1);
	}

	ldap_host = argv[1];
	hostname = argv[2];
	realm = argv[3];

	ads = ads_init(realm, ldap_host, NULL);

	rc = ads_connect(ads);

	rc = ads_join_realm(ads, hostname);
	return 0;
}
