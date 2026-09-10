# Adventure (350 points), PDP-8 / OS-8 FORTRAN IV -- native port.
#
# Needs a C compiler and zlib.  On Windows the Strawberry Perl mingw-w64
# toolchain provides both; elsewhere any gcc/clang with -lz will do.

CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra
LDFLAGS ?= -static
LDLIBS  ?= -lz
PYTHON  ?= python

EXE  = adventure.exe
PACK = ../src_original/advent-work.rk05

all: $(EXE)

$(EXE): src/pdp8.c src/rk05image.h
	$(CC) $(CFLAGS) -o $@ src/pdp8.c -Isrc $(LDFLAGS) $(LDLIBS)

src/rk05image.h: tools/mkimage.py $(PACK)
	$(PYTHON) tools/mkimage.py $(PACK) $@

test: $(EXE)
	$(PYTHON) tests/regress.py

clean:
	rm -f $(EXE) src/rk05image.h src/advent-trim.rk05
	rm -rf tests/work

.PHONY: all test clean
