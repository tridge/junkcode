#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <krb5.h>

#define DEBUG(l, x) printf x
#define NT_STATUS_LOGON_FAILURE -1
#define NT_STATUS_NO_MEMORY -1
#define NT_STATUS_OK 0

/****************************************************************************
load a file into memory from a fd.
****************************************************************************/ 

char *fd_load(int fd, size_t *size)
{
	struct stat sbuf;
	char *p;

	if (fstat(fd, &sbuf) != 0) return NULL;

	p = (char *)malloc(sbuf.st_size+1);
	if (!p) return NULL;

	if (read(fd, p, sbuf.st_size) != sbuf.st_size) {
		free(p);
		return NULL;
	}
	p[sbuf.st_size] = 0;

	if (size) *size = sbuf.st_size;

	return p;
}

/****************************************************************************
load a file into memory
****************************************************************************/
char *file_load(const char *fname, size_t *size)
{
	int fd;
	char *p;

	if (!fname || !*fname) return NULL;
	
	fd = open(fname,O_RDONLY);
	if (fd == -1) return NULL;

	p = fd_load(fd, size);

	close(fd);

	return p;
}

/*
  verify an incoming ticket and parse out the principal name and 
  authorization_data if available 
*/
static int verify_ticket(const char *ticket, size_t tsize, char *password_s)
{
	krb5_context context;
	krb5_auth_context auth_context = NULL;
	krb5_keytab keytab = NULL;
	krb5_data packet;
	krb5_ticket *tkt = NULL;
	krb5_data salt;
	krb5_encrypt_block eblock;
	int ret, i;
	krb5_keyblock * key;
	krb5_principal host_princ;
	char *host_princ_s;
	char *myname = "blu";
	char *realm = "FLAGSHIP.DOT-NET";
	krb5_data password;
	krb5_enctype *enctypes = NULL;

	password.data = password_s;
	password.length = strlen(password_s);

	ret = krb5_init_context(&context);
	if (ret) {
		DEBUG(1,("krb5_init_context failed (%s)\n", error_message(ret)));
		return -1;
	}

	ret = krb5_set_default_realm(context, realm);
	if (ret) {
		DEBUG(1,("krb5_set_default_realm failed (%s)\n", error_message(ret)));
		return -1;
	}

	/* this whole process is far more complex than I would
           like. We have to go through all this to allow us to store
           the secret internally, instead of using /etc/krb5.keytab */
	ret = krb5_auth_con_init(context, &auth_context);
	if (ret) {
		DEBUG(1,("krb5_auth_con_init failed (%s)\n", error_message(ret)));
		return -1;
	}

	asprintf(&host_princ_s, "HOST/%s@%s", myname, realm);
	ret = krb5_parse_name(context, host_princ_s, &host_princ);
	if (ret) {
		DEBUG(1,("krb5_parse_name(%s) failed (%s)\n", host_princ_s, error_message(ret)));
		return -1;
	}

	ret = krb5_principal2salt(context, host_princ, &salt);
	if (ret) {
		DEBUG(1,("krb5_principal2salt failed (%s)\n", error_message(ret)));
		return NT_STATUS_LOGON_FAILURE;
	}
    
	if (!(key = (krb5_keyblock *)malloc(sizeof(*key)))) {
		return NT_STATUS_NO_MEMORY;
	}

	if ((ret = krb5_get_permitted_enctypes(context, &enctypes))) {
		DEBUG(1,("krb5_get_permitted_enctypes failed (%s)\n", 
			 error_message(ret)));
		return NT_STATUS_LOGON_FAILURE;
	}

	for (i=0;enctypes[i];i++) {
		krb5_use_enctype(context, &eblock, enctypes[i]);

		ret = krb5_string_to_key(context, &eblock, key, &password, &salt);
		if (ret) {
			continue;
		}

		krb5_auth_con_setuseruserkey(context, auth_context, key);

		packet.length = tsize;
		packet.data = (krb5_pointer)ticket;

		if (!(ret = krb5_rd_req(context, &auth_context, &packet, 
				       NULL, keytab, NULL, &tkt))) {
			krb5_free_ktypes(context, enctypes);
			return NT_STATUS_OK;
		}
	}

	DEBUG(1,("krb5_rd_req failed (%s)\n", 
		 error_message(ret)));

	krb5_free_ktypes(context, enctypes);

	return NT_STATUS_LOGON_FAILURE;
}


int main(int argc, char *argv[])
{
	char *tfile = argv[1];
	char *pass = argv[2];
	char *ticket;
	size_t tsize;

	ticket = file_load(tfile, &tsize);
	if (!ticket) {
		perror(tfile);
		exit(1);
	}
	
	return verify_ticket(ticket, tsize, pass);
}
