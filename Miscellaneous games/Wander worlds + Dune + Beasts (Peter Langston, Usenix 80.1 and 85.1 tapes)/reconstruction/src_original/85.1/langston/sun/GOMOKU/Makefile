CFLAGS = -O
#LIBS should be "-lS" for V6, "-lm" for V7 or BSD
LIBS = -lm

all: gomoku gom_txt install

install: gomoku
	../install gomoku

gomoku: gomoku.o gomsys.o
	$(CC) $(CFLAGS) gomoku.o gomsys.o ../libtcap.a ../glib.a $(LIBS) -o $@
	@echo "$@ is ready"

gomsys.o: gomsys.c
	$(CC) $(CFLAGS) -c $*.c

gom_txt: gom_txt.n
	nroff gom_txt.n >$@
