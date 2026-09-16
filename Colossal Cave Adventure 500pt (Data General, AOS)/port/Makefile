CC     = gcc
CFLAGS = -O2 -Wall -Wextra -std=c99

all: aosvs16.exe adventure.exe dis.exe

aosvs16.exe: src/cpu.c src/eclipse.h src/debug.h src/fpu.h src/fpuexec.h src/cis.h
	$(CC) $(CFLAGS) -o $@ src/cpu.c

# The same program; it finds data/ADVENTURE.PR by itself.
adventure.exe: aosvs16.exe
	cp aosvs16.exe $@

dis.exe: src/dis.c src/eclipse.h
	$(CC) $(CFLAGS) -o $@ src/dis.c

test: aosvs16.exe
	sh tests/run.sh

clean:
	rm -f aosvs16.exe adventure.exe dis.exe

.PHONY: all test clean
