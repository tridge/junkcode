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

	if (argc != 4) {
		usage();
		exit(1);
	}

	ldap_host = argv[1];
	hostname = argv[2];
	realm = argv[3];

	asprintf(&principal, "HOST/%s@%s", hostname, realm);

	ldap_add_machine_account(ldap_host, hostname, realm);

	krb5_set_principal_password(principal, ldap_host, hostname, realm);

	return 0;
}
