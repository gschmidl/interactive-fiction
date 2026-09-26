CFLAGS = -O

all: oracle install

install: oracle
	../install oracle

oracle: oracle.o oraglb.o
	$(CC) $(CFLAGS) oracle.o oraglb.o ../glib.a -o $@
	chmod 4755 $@
	@echo "$@ is ready"

oraglb.o: oraglb.c oracle.h
	$(CC) $(CFLAGS) -c $*.c
