#include "includes.h"

int main(int argc, char *argv[])
{
	ADS_STRUCT *ads;
	LDAPMessage *res;
	char *ldap_host = argv[1];
	char *realm = argv[2];
	char *host = argv[3];
	int rc;

	ads = ads_init(realm, ldap_host, NULL);
	ads_connect(ads);

	rc = ads_find_machine_acct(ads, &res, host);
	ads_dump(ads, res);

	return rc;
}
