CFLAGS = -O
INCL = -I..

all: convoy install

install: convoy
	../install convoy

convoy: convoy.o conglb.o ../glib.a ../libtcap.a
	$(CC) $(CFLAGS) $@.o conglb.o ../glib.a ../libtcap.a -lm -o $@
	chmod 4755 $@
	@echo "$@ is ready"

conglb.o: conglb.c convoy.h
	$(CC) $(CFLAGS) $(INCL) -c conglb.c
