K5INST=/usr/kerberos

CFLAGS=-Wall -g -I$(LDAP)/include -I$(K5INST)/include 
LIBS = -L$(K5INST)/lib -lkrb5 -lk5crypto -lresolv -lcom_err -lldap 

all: netjoin dumphost

OBJ = setpw.o keytab.o asn1.o util_ads.o

netjoin: $(OBJ) netjoin.o
	$(CC) $(CFLAGS) -o $@ netjoin.o $(OBJ) $(LIBS)

dumphost: dumphost.o $(OBJ)
	$(CC) $(CFLAGS) -o $@ dumphost.o $(OBJ) $(LIBS)

clean:
	/bin/rm -f *.o netjoin *~
