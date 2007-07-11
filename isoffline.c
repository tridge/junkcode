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


/* these typedefs and structures from Martin Petermann
   <martin.petermann@de.ibm.com> */
#define MID_EXTOBJID_LEN         28

typedef struct s_midExtObjId
{
	unsigned char id[MID_EXTOBJID_LEN];
} midExtObjId_t;

typedef struct mi_flags
{
    unsigned int  f1;            /* flags field #1 */
    unsigned int  f2;            /* flags field #2 */
} mi_flags_t;

typedef uint32_t dsUint32_t;
typedef int32_t dsInt32_t;

struct dmiInfo {
   midExtObjId_t mi_extObjId;    /* External Object ID */
   dm_size_t     mi_logicalSize; /* Resident File's logical size */
   dm_size_t     mi_blkCnt;      /* Resident File's block count */
   dsUint32_t    mi_dataSizeHi;  /* High 32-bit of size of file data for this stub */
   dsUint32_t    mi_dataSize;    /* Size of file data for this stub */
   dsUint32_t    mi_serverNum;   /* Server number as per location srvc*/
   mi_flags_t    mi_flags;       /* miginfo's flag fields */
   dsUint32_t    mi_data;        /* not used for DMAPI     */
   dsUint32_t    mi_chkSum;      /* not used for DMAPI     */
   dsInt32_t     resStat;        /* migration status of the file */
   /* remaining fields deleted - not needed for this code */
};

#define FSM_MIG_NONRES_FLAG  2  /* File is non-resident (stub) */

#define DM_ATTRIB_OBJECT "IBMObj"



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
	struct dm_attrlist *a;
	int ret;
	uint32_t resStat;
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

	/* we need to keep looping while the memory we pass is too small */
	do {
		ret = dm_getall_dmattr(state.sid, handle, handle_len, 
				       DM_NO_TOKEN, state.buflen,
				       state.buf, &rlen);
		if (ret == -1 && errno == E2BIG) {
			size_t newlen = 2*(state.buflen+512);
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

	/* walk the list of attributes, trying to find the IBMObj one */
	for (a=state.buf;a;a=DM_STEP_TO_NEXT(a, struct dm_attrlist *)) {
		if (strcmp(a->al_name.an_chars, DM_ATTRIB_OBJECT) == 0) {
			/* found it, get the offline bit, and we're done */
			struct dmiInfo *dmi = DM_GET_VALUE(a, al_data, struct dmiInfo *);
			resStat = dmi->resStat;
			dm_handle_free(handle, handle_len);
			*offline = ((resStat & FSM_MIG_NONRES_FLAG) != 0);
			return 0;
		}
	}

	/* didn't find the IBMObj attribute, must be online */
	dm_handle_free(handle, handle_len);
	*offline = false;
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
