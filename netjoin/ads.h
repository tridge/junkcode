/*
  header for ads (active directory) library routines

  basically this is a wrapper around ldap
*/

typedef struct {
	LDAP *ld;
	char *realm;
	char *ldap_server;
	char *kdc_server;
	int ldap_port;
	char *bind_path;
} ADS_STRUCT;

