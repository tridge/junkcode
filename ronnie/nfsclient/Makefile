
all: nfsclient nfsio

clean:
	rm -f *.o
	rm -f nfsclient

nfsio: nfsio.o mount_xdr.o mount_client.o nfs_xdr.o nfs_client.o
	gcc -o $@ nfsio.o mount_client.o mount_xdr.o nfs_client.o nfs_xdr.o -lpthread

nfsclient: nfsclient.o mount_xdr.o mount_client.o nfs_xdr.o nfs_client.o
	gcc -o $@ nfsclient.o mount_client.o mount_xdr.o nfs_client.o nfs_xdr.o 

nfsio.o: nfsio.c
	gcc -g -c nfsio.c -o $@

nfsclient.o: nfsclient.c
	gcc -g -c nfsclient.c -o $@

mount_xdr.o:  mount_xdr.c
	gcc -g -c mount_xdr.c -o $@

mount_client.o:  mount_client.c
	gcc -g -c mount_client.c -o $@

mount.h:  mount.x
	@echo Generating $@
	rpcgen -h mount.x > mount.h

mount_xdr.c:  mount.x mount.h
	@echo Generating $@
	rpcgen -c mount.x > mount_xdr.c

mount_client.c:  mount.x mount.h
	@echo Generating $@
	rpcgen -l mount.x > mount_client.c

nfs_xdr.o:  nfs_xdr.c
	gcc -g -c nfs_xdr.c -o $@

nfs_client.o:  nfs_client.c
	gcc -g -c nfs_client.c -o $@

nfs.h:  nfs.x
	@echo Generating $@
	rpcgen -h nfs.x > nfs.h

nfs_xdr.c:  nfs.x nfs.h
	@echo Generating $@
	rpcgen -c nfs.x > nfs_xdr.c

nfs_client.c:  nfs.x nfs.h
	@echo Generating $@
	rpcgen -l nfs.x > nfs_client.c


