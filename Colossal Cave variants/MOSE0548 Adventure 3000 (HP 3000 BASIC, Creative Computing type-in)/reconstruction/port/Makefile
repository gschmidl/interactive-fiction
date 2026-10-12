# ADVENTURE/3000 (Creative Computing, November 1979) on a BASIC/3000 interpreter.
CC := $(if $(filter default,$(origin CC)),gcc,$(CC))
CFLAGS  ?= -std=c99 -O2 -Wall -Wextra
LDLIBS  ?= -lm
EXE     ?= adventure3000.exe

$(EXE): src/hp3kbasic.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

test: $(EXE)
	sh tests/run_tests.sh

clean:
	rm -f $(EXE) data/*/*.dat tests/data/*.dat tests/out_*.txt tests/diff_*.txt
.PHONY: test clean
