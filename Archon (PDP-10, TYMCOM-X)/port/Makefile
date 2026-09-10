CC      = gcc
CFLAGS  = -O2 -Wall -Wno-unused-result -std=gnu99
OBJ     = src/cpu.o src/monitor.o src/main.o src/image.o

bin/archon.exe: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

src/%.o: src/%.c src/pdp10.h
	$(CC) $(CFLAGS) -c -o $@ $<

src/image.c: ../dump_original/archon.low ../dump_original/archon.shr tools/mkimage.py
	python tools/mkimage.py

clean:
	rm -f src/*.o bin/archon.exe

.PHONY: clean
