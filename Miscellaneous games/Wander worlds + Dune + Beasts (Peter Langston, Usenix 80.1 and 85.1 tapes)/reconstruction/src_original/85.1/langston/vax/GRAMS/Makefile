CFLAGS = -O

all: grams degram install
	touch gramsrec
	chmod 664 gramsrec

install: grams
	../install grams

degram: degram.o gramglb.o
	$(CC) $(CFLAGS) degram.o gramglb.o ../glib.a -o $@
	chmod 2755 $@
	@echo "$@ is ready"

grams: grams.o gramglb.o
	$(CC) $(CFLAGS) grams.o gramglb.o ../glib.a -o $@
	chmod 2755 $@
	@echo "$@ is ready"

gramglb.o: gramglb.c gramdef.h
	$(CC) $(CFLAGS) -c $*.c
