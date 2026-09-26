CC      = gcc
CFLAGS  = -O2 -Wall -Wno-unused-result -std=gnu99
OBJ     = src/cpu.o src/monitor.o src/main.o src/image.o

bin/haunt.exe: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

src/%.o: src/%.c src/pdp10.h
	$(CC) $(CFLAGS) -c -o $@ $<

src/image.c: ../work/HAUNT.EXE.extracted tools/mkimage.py
	python tools/mkimage.py

clean:
	rm -f src/*.o bin/haunt.exe

.PHONY: clean
