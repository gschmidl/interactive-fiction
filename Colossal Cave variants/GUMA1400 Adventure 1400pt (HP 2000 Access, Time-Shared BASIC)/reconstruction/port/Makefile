# HP 2000 Access Time-Shared BASIC interpreter + Adventure ]I[ launcher.
CC      ?= gcc
CFLAGS  ?= -std=c99 -O2 -Wall -Wextra
LDLIBS  ?= -lm
EXE     ?= advent.exe

$(EXE): src/hptsb.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

test: $(EXE)
	sh tests/regress.sh

clean:
	rm -f $(EXE)

distclean: clean
	rm -rf data

.PHONY: test clean distclean
