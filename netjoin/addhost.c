/*--

  add a host as a machine account to a ldap server
  Written by tridge@samba.org


  inspired by code from AD.tgz from Microsoft
  
--*/

#include "includes.h"
#include <sasl.h>

/* return a dn of the form "dc=AA,dc=BB,dc=CC" from a 
   realm of the form AA.BB.CC */
static char *build_dn(const char *realm)
{
	char *p, *r;
	int numdots = 0;
	char *ret;
	
	r = strdup(realm);

	for (p=r; *p; p++) {
		if (*p == '.') numdots++;
	}

	ret = malloc((numdots+1)*4 + strlen(r) + 1);
	strcpy(ret,"dc=");
	p=strtok(r,"."); 
	strcat(ret, p);

	while ((p=strtok(NULL,"."))) {
		strcat(ret,",dc=");
		strcat(ret, p);
	}

	free(r);

	return ret;
}


static int find_host(LDAP *ld, LDAPMessage **res, 
		     const char *bind_path, const char *host)
{
	int ret;
	char *exp;

	asprintf(&exp, "(samAccountName=%s$)", host);
	*res = NULL;
	ret = ldap_search_s(ld, bind_path, LDAP_SCOPE_SUBTREE, exp, NULL, 0, res);
	free(exp);
	return ret;
}

#define MAX_MOD_VALUES 10

/*
  a convenient routine for adding a generic LDAP record 
*/
static int gen_ldap_add(LDAP *ld, const char *new_dn, ...)
{
	int i;
	va_list ap;
	LDAPMod **mods;
	char *name, *value;
	int ret;
	
	/* count the number of attributes */
	va_start(ap, new_dn);
	for (i=0; va_arg(ap, char *); i++) {
		/* skip the values */
		while (va_arg(ap, char *)) ;
	}
	va_end(ap);

	mods = malloc(sizeof(LDAPMod *) * (i+1));

	va_start(ap, new_dn);
	for (i=0; (name=va_arg(ap, char *)); i++) {
		char **values;
		int j;
		values = (char **)malloc(sizeof(char *) * (MAX_MOD_VALUES+1));
		for (j=0; (value=va_arg(ap, char *)) && j < MAX_MOD_VALUES; j++) {
			values[j] = value;
		}
		values[j] = NULL;
		mods[i] = malloc(sizeof(LDAPMod));
		mods[i]->mod_type = name;
		mods[i]->mod_op = LDAP_MOD_ADD;
		mods[i]->mod_values = values;
	}
	mods[i] = NULL;
	va_end(ap);

	ret = ldap_add_s(ld, new_dn, mods);

	for (i=0; mods[i]; i++) {
		free(mods[i]->mod_values);
		free(mods[i]);
	}
	free(mods);
	
	return ret;
}


static int add_host(LDAP *ld,char *bind_path, 
		    const char *hostname, const char *domainname)
{
	int ret;
	char *host_spn, *host_upn, *new_dn, *samAccountName, *controlstr;

	asprintf(&host_spn, "HOST/%s", hostname);
	asprintf(&host_upn, "%s@%s", host_spn, domainname);
	asprintf(&new_dn, "cn=%s,cn=Computers,%s", hostname, bind_path);
	asprintf(&samAccountName, "%s$", hostname);
	asprintf(&controlstr, "%u", 
		UF_DONT_EXPIRE_PASSWD | UF_WORKSTATION_TRUST_ACCOUNT |
		UF_TRUSTED_FOR_DELEGATION | UF_USE_DES_KEY_ONLY);
    
	ret = gen_ldap_add(ld, new_dn,
			   "cn", hostname, NULL,
			   "sAMAccountName", samAccountName, NULL,
			   "objectClass", 
			      "top", "person", "organizationalPerson", 
			      "user", "computer", NULL,
			   "userPrincipalName", host_upn, NULL, 
			   "servicePrincipalName", host_spn, NULL,
			   "dNSHostName", hostname, NULL,
			   "userAccountControl", controlstr, NULL,
			   "operatingSystem", "Samba", NULL,
			   "operatingSystemVersion", VERSION, NULL,
			   NULL);

	free(host_spn);
	free(host_upn);
	free(new_dn);
	free(samAccountName);
	free(controlstr);

	return ret;
}

#if 0
static void ldap_dump(LDAP *ld,LDAPMessage *res)
{
	LDAPMessage *e;
    
	for (e = ldap_first_entry(ld, res); e; e = ldap_next_entry(ld, e)) {
		BerElement *b;
		char *attr;
		char *dn = ldap_get_dn(ld, res);
		if (dn)
			printf("dn: %s\n", dn);
		ldap_memfree(dn);

		for (attr = ldap_first_attribute(ld, e, &b); 
		     attr;
		     attr = ldap_next_attribute(ld, e, b)) {
			char **values, **p;
			values = ldap_get_values(ld, e, attr);
			for (p = values; *p; p++) {
				printf("%s: %s\n", attr, *p);
			}
			ldap_value_free(values);
			ldap_memfree(attr);
		}

		ber_free(b, 1);
		printf("\n");
	}
}
#endif


static int sasl_interact(LDAP *ld,unsigned flags,void *defaults,void *in)
{
	sasl_interact_t *interact = in;

	while (interact->id != SASL_CB_LIST_END) {
		interact->result = strdup("");
		interact->len = 0;
		interact++;
	}
	
	return LDAP_SUCCESS;
}

int ldap_add_machine_account(const char *ldap_host, 
			     const char *hostname, const char *realm)
{
	LDAP *ld;
	int ldap_port = LDAP_PORT;
	char *bind_path;
	int rc;
	LDAPMessage *res;
	void *sasl_defaults;
	int version = LDAP_VERSION3;

	bind_path = build_dn(realm);

	printf("Creating host account for %s@%s\n", hostname, realm);

	ld = ldap_open(ldap_host, ldap_port);
	ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version);

	rc = ldap_sasl_interactive_bind_s(ld, NULL, NULL, NULL, NULL, 0,
					  sasl_interact, NULL);

	if (rc != LDAP_SUCCESS) {
		ldap_perror(ld, "ldap_bind");
		goto failed;
	}

	rc = find_host(ld, &res, bind_path, hostname);
	if (rc == LDAP_SUCCESS && ldap_count_entries(ld, res) == 1) {
		printf("Host account for %s already exists\n", hostname);
		goto finished;
	}

	rc = add_host(ld, bind_path, hostname, realm);
	if (rc != LDAP_SUCCESS) {
		ldap_perror(ld, "add_host");
		goto failed;
	}

	rc = find_host(ld, &res, bind_path, hostname);
	if (rc != LDAP_SUCCESS || ldap_count_entries(ld, res) != 1) {
		ldap_perror(ld, "find_host test");
		goto failed;
	}

	printf("Successfully added machine account for %s\n", hostname);

finished:	
	free(bind_path);
	return 0;

failed:
	printf("ldap_add_machine_account failed\n");
	free(bind_path);
	ldap_unbind(ld);
	return 1;
}
