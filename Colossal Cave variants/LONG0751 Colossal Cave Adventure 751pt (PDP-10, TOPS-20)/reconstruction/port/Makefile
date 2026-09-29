# ADVENTURE 751 -- native Windows build.
#
# make          builds bin/adv751.exe
# make test     replays the test scripts and compares them with the
#               transcripts taken off the original TOPS-20 system
# make clean    removes the objects and the binary
# make realclean also removes the generated image sources

CC      = gcc
CFLAGS  = -O2 -Wall -Wno-unused-result -Isrc
OBJ     = src/cpu.o src/monitor.o src/jsys.o src/main.o src/image.o src/data.o

all: bin/adv751.exe

bin/adv751.exe: $(OBJ)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $(OBJ)

$(OBJ): src/pdp10.h

# The game, the FORTRAN runtime and the databases come out of the files
# taken off the pack; see tools/mkimage.py.
src/image.c src/data.c: tools/mkimage.py
	python tools/mkimage.py

test: bin/adv751.exe
	python tests/compare.py

clean:
	rm -f src/*.o bin/adv751.exe

realclean: clean
	rm -f src/image.c src/data.c
