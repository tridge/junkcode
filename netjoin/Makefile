K5INST=/home/tridge/kerberos

CFLAGS=-Wall -g -I$(LDAP)/include -I$(K5INST)/include 
LIBS = -L$(K5INST)/lib -lkrb5 -lk5crypto -lresolv -lcom_err -lldap -llber -lsasl -ldl -lsasl -lssl -lcrypto

all: netjoin 

OBJ = setpw.o addhost.o keytab.o asn1.o

netjoin: $(OBJ) netjoin.o
	$(CC) $(CFLAGS) -o $@ netjoin.o $(OBJ) $(LIBS)

clean:
	/bin/rm -f *.o netjoin *~
