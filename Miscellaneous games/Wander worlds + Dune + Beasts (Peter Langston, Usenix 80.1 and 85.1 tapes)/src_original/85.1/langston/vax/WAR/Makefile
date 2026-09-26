CFLAGS = -O -I..
TCAPLIB = ../libtcap.a
MISC = -DEMPTY

all: war awar warinit caat install

install: war awar
	../install war awar

export: wardef.h war.c warglb.c warguts.o awar.c warinit.o

war:    war.o warglb.o warguts.o
	$(CC) $(CFLAGS) $@.o warglb.o warguts.o $(TCAPLIB) ../glib.a -o $@
	@echo "$@ is ready"

awar:   awar.o warglb.o warguts.o
	$(CC) $(CFLAGS) $@.o warglb.o warguts.o $(TCAPLIB) ../glib.a -o $@
	@echo "$@ is ready"

warinit: warinit.o warglb.o
	$(CC) $(CFLAGS) $@.o warglb.o ../glib.a -o $@
	@echo "$@ is ready"

caat: caat.c
	$(CC) $(CFLAGS) $@.c -o $@
	@echo "$@ is ready"

warglb.o: warglb.c wardef.h
	$(CC) $(CFLAGS) -c warglb.c

war.o: war.c wardef.h
	$(CC) $(CFLAGS) $(MISC) -c war.c

awar.o: awar.c wardef.h
	$(CC) $(CFLAGS) -c awar.c
