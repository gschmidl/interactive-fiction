all: mm install

install: mm
	../install mm

mm: mm.o ../glib.a
	$(CC) $(CFLAGS) mm.o ../glib.a -lm -o $@
	@echo "$@ is ready"
