# WANG 928 ADVENTURE -- Windows port
#
#   mingw32-make            build wangadv.exe and wangtest.exe
#   mingw32-make run        build and start the game
#   mingw32-make check      replay tools/demo.txt through the scripted front end
#   mingw32-make clean

CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -std=c99
SRC     = src/wang.c src/z80.c
HDRS    = src/wang.h src/z80.h

all: wangadv.exe wangtest.exe

wangadv.exe: $(SRC) src/con_win.c $(HDRS)
	$(CC) $(CFLAGS) -o $@ $(SRC) src/con_win.c

wangtest.exe: $(SRC) src/con_test.c $(HDRS)
	$(CC) $(CFLAGS) -o $@ $(SRC) src/con_test.c

run: wangadv.exe
	./wangadv.exe

check: wangtest.exe
	./wangtest.exe < tools/demo.txt

clean:
	rm -f wangadv.exe wangtest.exe

.PHONY: all run check clean
