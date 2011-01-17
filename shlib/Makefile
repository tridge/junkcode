CFLAGS=-Wall

SONAME1=libversion1.so
SONAME2=libversion2.so
SYMVER1=symversion1
SYMVER2=symversion2

all: main public/module.so

private:
	mkdir -p private

public:
	mkdir -p public

vscript1:
	echo "$(SYMVER1) { global: *; };" > vscript1

vscript2:
	echo "$(SYMVER2) { global: *; };" > vscript2

public/$(SONAME1): lib.c vscript1 public
	gcc $(CFLAGS) -Wl,-soname=$(SONAME1) -Wl,--version-script=vscript1 -DLIBVERSION=\"version1\" -fPIC -shared lib.c -o public/$(SONAME1)

private/$(SONAME2): lib.c vscript2 private
	gcc $(CFLAGS) -Wl,-soname=$(SONAME2) -Wl,--version-script=vscript2 -DLIBVERSION=\"version2\" -fPIC -shared lib.c -o private/$(SONAME2)

public/module.so: module.c private/$(SONAME2)
	gcc $(CFLAGS) -Wl,--rpath=$(PWD)/private -fPIC -shared module.c -o public/module.so private/$(SONAME2)

main: main.c public/$(SONAME1) public/module.so
	gcc $(CFLAGS) -Wl,--rpath=$(PWD)/public main.c -o main public/$(SONAME1) -ldl

clean:
	rm -rf *.o *.so *~ private public main vscript*


