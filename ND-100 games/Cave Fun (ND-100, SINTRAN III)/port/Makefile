CC     = gcc
CFLAGS = -O2 -Wall -Wextra -std=c99
SRC    = src/main.c src/cpu.c src/fpu.c src/dis.c src/sintran.c src/term.c
HDR    = src/nd100.h src/sintran.h src/term.h

cavefun.exe: $(SRC) $(HDR)
	$(CC) $(CFLAGS) -DGAME=4 -o $@ $(SRC)

clean:
	rm -f cavefun.exe

.PHONY: clean
