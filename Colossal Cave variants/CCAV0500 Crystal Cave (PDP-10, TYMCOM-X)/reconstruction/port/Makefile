CC      = gcc
CFLAGS  = -O2 -Wall -Wno-unused-result -std=gnu99
OBJ     = src/cpu.o src/monitor.o src/main.o src/image.o
BUILD   = 1979

# 1979 is the build that gets the plain name; the two later ones are
# named for their year.  Each has its own core image, so switching
# BUILD means regenerating src/image.c -- which is what "builds" does.
all: bin/cave.exe

bin/cave.exe: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

bin/cave-$(BUILD).exe: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

src/%.o: src/%.c src/pdp10.h
	$(CC) $(CFLAGS) -c -o $@ $<

src/image.c: raw/cave-upl20.sav raw/cave-upl22.sav tools/mkimage.py
	python tools/mkimage.py --build=$(BUILD)

# All three side by side.  Each pass deletes the generated image first,
# so make cannot decide that a different BUILD= from the previous pass
# is still up to date: timestamps here have one-second resolution and a
# build takes less.
builds:
	@rm -f src/image.c src/image.o
	@$(MAKE) --no-print-directory BUILD=1979 bin/cave.exe
	@for b in 1983 1984; do \
	    rm -f src/image.c src/image.o; \
	    $(MAKE) --no-print-directory BUILD=$$b bin/cave-$$b.exe || exit 1; \
	done
	@rm -f src/image.c src/image.o
	@python tools/mkimage.py --build=$(BUILD) >/dev/null

clean:
	rm -f src/*.o

distclean: clean
	rm -f src/image.c bin/cave.exe bin/cave-*.exe

.PHONY: all builds clean distclean
