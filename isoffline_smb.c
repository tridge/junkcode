/*
  demonstration of checking if a file is offline using DMAPI, with
  shortcut tricks using st_atime and st_blocks

  Build with
      gcc -o isoffline isoffline.c -ldmapi

  Andrew Tridgell (tridgell@au1.ibm.com) July 2007
 */

#include <stdio.h>
#include <dmapi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>

/* optimisation tunables - used to avoid the DMAPI slow path */
#define FILE_IS_ONLINE_RATIO      0.5
#define FILE_IS_ONLINE_ATIME      60

#define DMAPI_SESSION_NAME "samba"
#define DMAPI_TRACE 10

static dm_sessid_t samba_dmapi_session = DM_NO_SESSION;

#define DEBUG(lvl, x) printf x
#define DEBUGADD(lvl, x) printf x

/* 
   Initialise DMAPI session. The session is persistant kernel state,
   so it might already exist, in which case we merely want to
   reconnect to it. This function should be called as root.
 */
static int dmapi_init_session(void)
{
	char	buf[DM_SESSION_INFO_LEN];
	size_t	buflen;
	uint	    nsessions = 5;
	dm_sessid_t *sessions = NULL;
	int i, err;
	char *version;

	if (dm_init_service(&version) < 0) {
		DEBUG(0,("dm_init_service failed - disabling DMAPI\n"));
		return -1;
	}

	ZERO_STRUCT(buf);

	do {
		dm_sessid_t *new_sessions;
		nsessions *= 2;
		new_sessions = TALLOC_REALLOC_ARRAY(NULL, sessions, 
						    dm_sessid_t, nsessions);
		if (new_sessions == NULL) {
			talloc_free(sessions);
			return -1;
		}
		sessions = new_sessions;
		err = dm_getall_sessions(nsessions, sessions, &nsessions);
	} while (err == -1 && errno == E2BIG);

	if (err == -1) {
		DEBUGADD(DMAPI_TRACE,
			("failed to retrieve DMAPI sessions: %s\n",
			strerror(errno)));
		talloc_free(sessions);
		return -1;
	}

	for (i = 0; i < nsessions; ++i) {
		err = dm_query_session(sessions[i], sizeof(buf), buf, &buflen);
		buf[sizeof(buf) - 1] = '\0';
		if (err == 0 && strcmp(DMAPI_SESSION_NAME, buf) == 0) {
			samba_dmapi_session = sessions[i];
			DEBUGADD(DMAPI_TRACE,
				("attached to existing DMAPI session "
				 "named '%s'\n", buf));
			break;
		}
	}

	talloc_free(sessions);

	/* No session already defined. */
	if (samba_dmapi_session == DM_NO_SESSION) {
		err = dm_create_session(DM_NO_SESSION, DMAPI_SESSION_NAME,
					&samba_dmapi_session);
		if (err < 0) {
			DEBUGADD(DMAPI_TRACE,
				("failed to create new DMAPI session: %s\n",
				strerror(errno)));
			samba_dmapi_session = DM_NO_SESSION;
			return -1;
		}

		DEBUG(0,("created new DMAPI session named '%s' for %s\n", 
			 DMAPI_SESSION_NAME, version));
	}

	if (samba_dmapi_session != DM_NO_SESSION) {
		set_effective_capability(DMAPI_ACCESS_CAPABILITY);
	}

	/* 
	   Note that we never end the DMAPI session. It gets re-used
	   if possible
	 */

	return 0;
}


/* 
   return a pointer to our dmapi session if available
   This assumes you have called dmapi_have_session() first
 */
const void *dmapi_get_current_session(void) 
{
	if (samba_dmapi_session == DM_NO_SESSION) {
		return NULL;
	}
	return (void *)&samba_dmapi_session;
}

/*
  this must be the first dmapi call you make in Samba. It will initialise dmapi
  if available and tell you if you can get a dmapi session. This should be called in
  the client specific child process
 */
BOOL dmapi_have_session(void)
{
	static BOOL initialised;
	if (!initialised) {
		initialised = true;
		become_root();
		dmapi_init_session();
		unbecome_root();
	}
	return samba_dmapi_session != DM_NO_SESSION;
}


/*
  see if a file is offline

  return -1 on failure. Set *offline to true/false according to
  offline status
 */
static int is_offline(char *fname, time_t now, bool *offline)
{
	struct stat st;
	void *handle=NULL;
	size_t handle_len=0;
	size_t rlen;
	int ret;
	dm_attrname_t dmAttrName;
	/* keep some state between calls, to save on session creation calls */
	static struct dmapi_state {
		dm_sessid_t sid;
		void *handle;
		size_t handle_len;
	} state;

	if (state.sid == 0) {
		/* create a new session if needed */
		if (dm_create_session(DM_NO_SESSION, "samba", &state.sid) != 0) {
			return -1;
		}
	}

	/* try shortcut methods first */
	if (stat(fname, &st) != 0) {
		return -1;
	}

	/* if the file has more than FILE_IS_ONLINE_RATIO of blocks available,
	   then assume its not offline (it may not be 100%, as it could be sparse) */
	if (512 * (off_t)st.st_blocks > st.st_size * FILE_IS_ONLINE_RATIO) {
		*offline = false;
		return 0;
	}

	/* go the slow DMAPI route */
	if (dm_path_to_handle(fname, &handle, &handle_len) != 0) {
		return -1;
	}

	memset(&dmAttrName, 0, sizeof(dmAttrName));
	strcpy((char *)&dmAttrName.an_chars[0], "IBMObj");

	ret = dm_get_dmattr(state.sid, handle, handle_len, 
			    DM_NO_TOKEN, &dmAttrName, 0, NULL, &rlen);

	/* its offline if the IBMObj attribute exists */
	*offline = (ret == 0 || (ret == -1 && errno == E2BIG));

	dm_handle_free(handle, handle_len);
	return 0;	
}


int main(int argc, char *argv[])
{
	int i;
	time_t now = time(NULL);
	now = time(NULL);
	if (argc < 2) {
		printf("isoffline <fname...>\n");
		exit(1);
	}
	for (i=1;i<argc;i++) {
		bool offline;
		if (is_offline(argv[i], now, &offline) == -1) {
			perror(argv[i]);
			exit(1);
		}
		printf("%s\t%s\n", offline?"offline":"online ", argv[i]);
	}
	return 0;
}
