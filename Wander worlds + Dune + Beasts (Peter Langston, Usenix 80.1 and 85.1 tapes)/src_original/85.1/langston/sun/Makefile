CFLAGS = -O
READY = @echo "$@ is ready"

all: replace sucker map

glib.a: GLIB/*.c
	(cd GLIB; make)

libtcap.a: TCAP/*.c
	(cd TCAP; make)

map: map.c
	$(CC) $(CFLAGS) $@.c -o $@
	$(READY)

replace: replace.c
	$(CC) $(CFLAGS) $@.c -o $@
	$(READY)

sucker: sucker.c
	$(CC) $(CFLAGS) $@.c glib.a -o $@
	chmod 4755 $@
	$(READY)
