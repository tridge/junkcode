/* 
   simple nfsclient

   Copyright (C) Ronnie Sahlberg 2007

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include "mount.h"
#include "nfs.h"

static fhandle3 *
get_mount_fh(const char *client, char *mntdir)
{
	CLIENT *clnt;
	dirpath mountdir=mntdir;
	mountres3 *mountres;
	fhandle3 *mfh;
	int i;

	clnt = clnt_create(client, MOUNT_PROGRAM, MOUNT_V3, "tcp");
	if (clnt == NULL) {
		printf("ERROR: failed to connect to MOUNT daemon on %s\n", client);
		exit(10);
	}

	mountres=mountproc3_mnt_3(&mountdir, clnt);
	if (mountres == NULL) {
		printf("ERROR: failed to call the MNT procedure\n");
		exit(10);
	}
	if (mountres->fhs_status != MNT3_OK) {
		printf("ERROR: Server returned error %d when trying to MNT\n",mountres->fhs_status);
		exit(10);
	}

	mfh = malloc(sizeof(fhandle3));
	mfh->fhandle3_len = mountres->mountres3_u.mountinfo.fhandle.fhandle3_len;
	mfh->fhandle3_val = malloc(mountres->mountres3_u.mountinfo.fhandle.fhandle3_len);
	memcpy(mfh->fhandle3_val, 
		mountres->mountres3_u.mountinfo.fhandle.fhandle3_val,
		mountres->mountres3_u.mountinfo.fhandle.fhandle3_len);

	clnt_destroy(clnt);

	printf("mount filehandle : %d ", mfh->fhandle3_len);
	for (i=0;i<mfh->fhandle3_len;i++) {
		printf("%02x", mfh->fhandle3_val[i]);
	}
	printf("\n");

	return mfh;
}


static nfs_fh3 *
lookup_fh(CLIENT *clnt, nfs_fh3 *dir, char *name)
{
	nfs_fh3 *fh;
	LOOKUP3args l3args;
	LOOKUP3res *l3res;
	int i;

	l3args.what.dir.data.data_len = dir->data.data_len;
	l3args.what.dir.data.data_val = dir->data.data_val;
	l3args.what.name = name;
	l3res = nfsproc3_lookup_3(&l3args, clnt);		
	if (l3res == NULL) {
		printf("Failed to lookup file %s\n", "x.dat");
		exit(10);
	}
	if (l3res->status != NFS3_OK) {
		printf("lookup returned error %d\n", l3res->status);
		exit(10);
	}

	fh = malloc(sizeof(nfs_fh3));
	fh->data.data_len = l3res->LOOKUP3res_u.resok.object.data.data_len;
	fh->data.data_val = malloc(fh->data.data_len);
	memcpy(fh->data.data_val, 
		l3res->LOOKUP3res_u.resok.object.data.data_val,
		fh->data.data_len);

	printf("file filehandle : %d ", fh->data.data_len);
	for (i=0;i<fh->data.data_len;i++) {
		printf("%02x", fh->data.data_val[i]);
	}
	printf("\n");

	return fh;
}

/* return number of bytes weitten    or -1 if there was an error */
static int
write_data(CLIENT *clnt, nfs_fh3 *fh, offset3 offset, char *buffer, 
		count3 count, enum stable_how stable)
{
	WRITE3args w3args;
	WRITE3res *w3res;

	w3args.file = *fh;
	w3args.offset = offset;
	w3args.count  = count;
	w3args.stable = stable;
	w3args.data.data_len = count;
	w3args.data.data_val = buffer;
	w3res = nfsproc3_write_3(&w3args, clnt);
	if (w3res == NULL) {
		return -1;
	}
	if (w3res->status != NFS3_OK) {
		return -1;
	}
	return w3res->WRITE3res_u.resok.count;
}


static void
commit_data(CLIENT *clnt, nfs_fh3 *fh)
{
	COMMIT3args c3args;
	COMMIT3res *c3res;

	c3args.file   = *fh;
	c3args.offset = 0;
	c3args.count  = 0;

	c3res = nfsproc3_commit_3(&c3args, clnt);
}

int main(int argc, const char *argv[])
{
	CLIENT *clnt;
	fhandle3 *mfh;
	nfs_fh3 *fh;
	char buffer[512];

	/* get the filehandle for the mountpoint */
	mfh = get_mount_fh("9.155.61.98", "/gpfs1/data");

	/* connect to NFS */
	clnt = clnt_create("9.155.61.98", NFS_PROGRAM, NFS_V3, "tcp");
	if (clnt == NULL) {
		printf("ERROR: failed to connect to NFS daemon on %s\n", "9.155.61.98");
		exit(10);
	}


	/* get the file filehandle */
	fh = lookup_fh(clnt, (nfs_fh3 *)mfh, "x.dat");

	
	if (write_data(clnt, fh, 1024, buffer, sizeof(buffer), UNSTABLE) == -1) {
		printf("failed to write data\n");
		exit(10);
	}
	commit_data(clnt, fh);

	return 0;
}
