CFLAGS = -O

all: dune install

install: dune
	../install dune

dune: dune.o duneglb.o
	$(CC) $(CFLAGS) dune.o duneglb.o ../glib.a ../libtcap.a -lm -o $@
	chmod 2755 $@
	@echo "$@ is ready"

doc: dune.doc
	imprint dune.doc
