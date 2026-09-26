CC     = gcc
CFLAGS = -O2 -Wall -Wextra -std=c99

all: dis.exe thissala.exe

dis.exe: src/dis.c src/eclipse.h
	$(CC) $(CFLAGS) -o $@ src/dis.c

thissala.exe: src/cpu.c src/eclipse.h src/debug.h src/fpu.h src/fpuexec.h
	$(CC) $(CFLAGS) -o $@ src/cpu.c

clean:
	rm -f dis.exe thissala.exe

.PHONY: all clean
