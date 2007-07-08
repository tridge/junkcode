CC = /usr/local/bin/gcc
LD = /usr/ccs/bin/ld
NM = /usr/ccs/bin/nm
CFLAGS = -dalign -Wa,-xarch=v8plusa -D_KERNEL -DSUNDDI

all: nfssrv zfs

nfssrv: hack.o nfssrv.hacked 
	$(LD) -o nfssrv nfssrv.hacked hack.o -r

zfs: hack2.o nfs.hacked 
	$(LD) -o zfs nfs.hacked hack2.o -r

hack2: hack2.o
	$(LD) -o hack2 hack2.o -r

nfssrv.fnlist:
	$(NM) nfssrv.orig | grep -v UNDEF|egrep 'FUNC|FILE|OBJT'|cut -d'|' -f8| egrep -v '^_|^.L' > nfssrv.fnlist

nfs.fnlist:
	$(NM) nfs.orig | grep -v UNDEF|egrep 'FUNC|FILE|OBJT'|cut -d'|' -f8| egrep -v '^_|^.L' > nfs.fnlist

nfssrv.hacked: nfssrv.orig nfssrv.fnlist nfssrv.hooks substr
	./hackit nfssrv.orig nfssrv.hacked nfssrv.fnlist nfssrv.hooks

nfs.hacked: nfs.orig nfs.fnlist nfs.hooks substr
	./hackit nfs.orig nfs.hacked nfs.fnlist nfs.hooks


substr: substr.c
	$(CC) -o substr substr.c

clean:
	rm -f nfssrv.fnlist nfs.fnlist nfssrv nfs *.o
