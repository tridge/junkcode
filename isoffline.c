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
#include <sys/stat.h>

/* optimisation tunables - used to avoid the DMAPI slow path */
#define FILE_IS_ONLINE_RATIO      0.5
#define FILE_IS_ONLINE_ATIME      60


/*
  see if a file is offline

  return -1 on failure. Set *offline to true/false according to
  offline status
 */
static int is_offline(char *fname, time_t now, bool *offline)
{
	struct stat st;
	void *handle;
	size_t handle_len;
	size_t rlen;
	int ret;
	dm_attrname_t dmAttrName;
	/* keep some state between calls, to save on session creation calls */
	static struct dmapi_state {
		dm_sessid_t sid;
		void *handle;
		size_t handle_len;
		void *buf;
		size_t buflen;	
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

	/* if the file was very recently accessed, assume its
	   online. This prevents us doing the expensive DMAPI calls a
	   lot on sparse files that are online */
	if (st.st_atime + FILE_IS_ONLINE_ATIME > now) {
		*offline = false;
		return 0;
	}

	/* go the slow DMAPI route */
	if (dm_path_to_handle(fname, &handle, &handle_len) != 0) {
		return -1;
	}

	memset(&dmAttrName, 0, sizeof(dmAttrName));
	strcpy((char *)&dmAttrName.an_chars[0], "IBMObj");

	/* we need to keep looping while the memory we pass is too small */
	do {
		ret = dm_get_dmattr(state.sid, handle, handle_len, 
				    DM_NO_TOKEN, &dmAttrName, state.buflen, state.buf, &rlen);
		if (ret == -1 && errno == E2BIG) {
			size_t newlen = 1.5*(state.buflen+32);
			void *newbuf = realloc(state.buf, newlen);
			if (newbuf == NULL) {
				errno = ENOMEM;				
				dm_handle_free(handle, handle_len);
				return -1;
			}
			state.buf = newbuf;
			state.buflen = newlen;
		}
	} while (ret == -1 && errno == E2BIG);

	/* its offline if the IBMObj attribute exists */
	*offline = (ret == 0);

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
