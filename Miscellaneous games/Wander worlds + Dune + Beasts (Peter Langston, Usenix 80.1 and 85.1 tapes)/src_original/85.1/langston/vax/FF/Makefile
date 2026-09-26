CFLAGS = -O
LIBS = ../glib.a

all:    ff init update fix rush install

install: ff
	../install ff

ff:     ff.o ffglb.o fflib.a
	$(CC) ff.o ffglb.o fflib.a $(LIBS) -o $@
	chmod 4755 ff
	@echo "$@ is ready"

init:   init.o ffglb.o fflib.a
	$(CC) init.o ffglb.o fflib.a $(LIBS) -o $@
	@echo "$@ is ready"

update: update.o ffglb.o fflib.a
	$(CC) update.o ffglb.o fflib.a $(LIBS) -o $@
	@echo "$@ is ready"

fix: fix.o ffglb.o fflib.a
	$(CC) fix.o ffglb.o fflib.a $(LIBS) -o $@
	@echo "$@ is ready"

rush: rush.o ffglb.o fflib.a
	$(CC) rush.o ffglb.o fflib.a $(LIBS) -o $@
	@echo "$@ is ready"

ranlib:
	ranlib fflib.a
