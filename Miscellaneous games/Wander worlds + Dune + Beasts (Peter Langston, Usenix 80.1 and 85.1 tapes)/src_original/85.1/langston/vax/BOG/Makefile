CFLAGS = -O -I.. -I/usr/include

# define one and only one of these...
#OPT = -DNODELAY  # if you have O_NDELAY
OPT = -DEMPTY    # if you have empty() or can fake it
#OPT = -DTWOPROC  # if you have neither empty() nor O_NDELAY

all: bog bogwds install

install: bog
	../install bog

bog: bog.o bogsys.o ../libtcap.a
	$(CC) $(CFLAGS) bog.o bogsys.o ../libtcap.a ../glib.a -o $@
	@echo "$@ is ready"

bogsys.o: bogsys.c
	$(CC) $(CFLAGS) -c $(OPT) $*.c

bogwds: bpack bw.bpack
	bpack -u <bw.bpack >bogwds
	@echo "You can delete bw.bpack now if you wish."

bpack: bpack.c
	$(CC) $(CFLAGS) $@.c -o $@
	@echo "$@ is ready"
