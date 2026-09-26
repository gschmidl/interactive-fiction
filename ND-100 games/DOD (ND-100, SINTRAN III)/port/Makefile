CC     = gcc
CFLAGS = -O2 -Wall -Wextra -std=c99
SRC    = src/main.c src/cpu.c src/fpu.c src/dis.c src/sintran.c src/term.c
HDR    = src/nd100.h src/sintran.h src/term.h

dod.exe: $(SRC) $(HDR)
	$(CC) $(CFLAGS) -DGAME=6 -o $@ $(SRC)

clean:
	rm -f dod.exe

.PHONY: clean
