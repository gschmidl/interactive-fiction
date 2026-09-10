# make's built-in CC is "cc", which mingw does not install, so override
# the default but still let the environment or command line win.
ifeq ($(origin CC),default)
CC      := gcc
endif
CFLAGS  ?= -std=c99 -Wall -Wextra -O2

# One emulator, two sets of images: the 1984 game and the later one.
COMMON   = src/main.c src/cpu.c src/monitor.c src/load.c
DUMP84   = ../dump_original
DUMP87   = ../dump_1987
BIN84    = bin/zarast.exe
BIN87    = bin/zarast87.exe

all: $(BIN84) $(BIN87)

$(BIN84): $(COMMON) src/images_84.c src/pdp10.h
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $(COMMON) src/images_84.c -lm

$(BIN87): $(COMMON) src/images_87.c src/pdp10.h
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $(COMMON) src/images_87.c -lm

# The images, embedded verbatim.  mkimages.py also works out where each
# one is a word short and refuses to emit an image it could not make
# self-consistent -- see docs/REVERSE-ENGINEERING.md.
src/images_84.c: tools/mkimages.py $(DUMP84)/dungen.shr $(DUMP84)/ventur.shr \
                 $(DUMP84)/charac.shr $(DUMP84)/charc.shr $(DUMP84)/filer.shr
	python tools/mkimages.py 84

src/images_87.c: tools/mkimages.py $(DUMP87)/pub.shr $(DUMP87)/b.shr \
                 $(DUMP87)/cr.shr $(DUMP84)/filer.shr
	python tools/mkimages.py 87

# The world as it stood on the tape, with its dropped word put back.
# Needs the built binary, because the reference is what FILER produces.
data/newadv-1984.dat: tools/mkworld.py $(BIN84) $(DUMP84)/newadv.dat
	python tools/mkworld.py

world: data/newadv-1984.dat

clean:
	rm -f $(BIN84) $(BIN87) src/*.o

.PHONY: all clean world
