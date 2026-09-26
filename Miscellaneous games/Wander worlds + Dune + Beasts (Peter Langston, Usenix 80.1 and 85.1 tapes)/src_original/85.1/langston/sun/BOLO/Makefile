BCC = bcc
CFLAGS = -O -I..
GLIB = ../glib.a
LIBTCAP = ../libtcap.a
EXES = bcc bdb bolo bds

.SUFFIXES:	.n .man .doc

.n.man:
	itroff -t $*.n >$*.man

.man.doc:
	lpr -Pimagen -t < $*.man
	-lpq

all: $(EXES) progs manuals install

progs: fred lump p1 p2

manuals: bcc.man bdb.man bref bom

install: $(EXES)
	../install $(EXES)

bcc: bcc.o gram.o
	$(CC) $(CFLAGS) bcc.o gram.o $(GLIB) -o $@
	@echo "$@ is ready"

bdb: bdb.o bguts.o
	$(CC) $(CFLAGS) $@.o bguts.o $(GLIB) -lm -o $@
	@echo "$@ is ready"

bds: bds.o
	$(CC) $(CFLAGS) $@.o $(GLIB) -o $@
	@echo "$@ is ready"

bolo: bolo.o bguts.o tcviz.o
	$(CC) $(CFLAGS) bolo.o bguts.o tcviz.o $(LIBTCAP) $(GLIB) -lm -o $@
	@echo "$@ is ready"

fred: fred.b bcc
	$(BCC) $@.b -o$@

lump: lump.b bcc
	$(BCC) $@.b -o$@

p1: p1.b bcc
	$(BCC) $@.b -o$@

p2: p2.b bcc
	$(BCC) $@.b -o$@
