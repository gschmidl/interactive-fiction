# make predefines CC as 'cc', which does not exist on a mingw box.
ifeq ($(origin CC),default)
CC = gcc
endif
CFLAGS  ?= -std=c99 -Wall -Wextra -O2
LDLIBS  ?= -lm

BIN     = bin/monastery.exe
SRC     = src/monastery.c src/basic.c
HDR     = src/basic.h src/gamedata.h

all: $(BIN)

$(BIN): $(SRC) $(HDR)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDLIBS)

# src/gamedata.h and docs/MAP.md are generated from the recovered originals.
regen:
	python ../work/extract_data.py
	python ../work/gen_data_h.py
	python ../work/gen_map.py

clean:
	rm -f $(BIN)

.PHONY: all regen clean
