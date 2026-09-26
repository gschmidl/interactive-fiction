CFLAGS = -O

all: beasts bsnoop install

install: beasts
	../install beasts

beasts: beasts.o beaglb.o
	$(CC) $(CFLAGS) beasts.o beaglb.o ../glib.a -o $@
	chmod 4755 $@
	@echo "$@ is ready"

beaglb.o: beasts.h

bsnoop: bsnoop.o beaglb.o
	$(CC) $(CFLAGS) bsnoop.o beaglb.o ../glib.a -o $@
	@echo "$@ is ready"
