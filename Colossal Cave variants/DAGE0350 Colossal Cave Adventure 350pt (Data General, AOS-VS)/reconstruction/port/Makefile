CC     = gcc
CFLAGS = -O2 -Wall -Wextra -std=c99

all: aosvs16.exe dis.exe

aosvs16.exe: src/cpu.c src/eclipse.h src/debug.h src/fpu.h src/fpuexec.h
	$(CC) $(CFLAGS) -o $@ src/cpu.c

dis.exe: src/dis.c src/eclipse.h
	$(CC) $(CFLAGS) -o $@ src/dis.c

clean:
	rm -f aosvs16.exe aosvs32.exe dis.exe

.PHONY: all clean
