CC      = gcc
CFLAGS  = -O2 -Wall -Wno-unused-result -std=gnu99
OBJ     = src/cpu.o src/monitor.o src/main.o src/image.o
BUILD   = 1979

# Every executable is named for the build it came from -- there is no
# plain "advent.exe", because it would only ever be a duplicate of one
# of these three.
all: bin/advent-$(BUILD).exe

bin/advent-$(BUILD).exe: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

src/%.o: src/%.c src/pdp10.h
	$(CC) $(CFLAGS) -c -o $@ $<

src/image.c: raw/advent-pointerc.sav raw/advent-upl20.sav raw/advent-games.sav \
             raw/advent-carl.sav raw/advent-upl17.sav tools/mkimage.py
	python tools/mkimage.py --build=$(BUILD)

# All three builds side by side, which is what tests/runall.sh drives.
# Each pass deletes the generated image first, so make cannot decide that
# a different BUILD= from the previous pass is still up to date: file
# timestamps here have one-second resolution and a build takes less.
builds:
	@for b in 1979 1981 1978; do \
	    rm -f src/image.c src/image.o; \
	    $(MAKE) --no-print-directory BUILD=$$b bin/advent-$$b.exe || exit 1; \
	done
	@rm -f src/image.c src/image.o
	@python tools/mkimage.py --build=$(BUILD) >/dev/null

test: builds
	sh tests/runall.sh

verify:
	python tools/mkimage.py --verify

clean:
	rm -f src/*.o

distclean: clean
	rm -f src/image.c bin/advent-*.exe

.PHONY: all clean distclean verify builds test
