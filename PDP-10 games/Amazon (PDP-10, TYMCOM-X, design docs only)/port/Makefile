# AMAZON -- Windows port.  Builds with MinGW-w64 gcc.
CC      = gcc
CFLAGS  = -std=c99 -O2 -Wall -Wextra
SRC     = src/main.c src/game.c src/world.c src/parse.c src/riddles.c src/share.c
OUT     = bin/amazon.exe

all: $(OUT)

$(OUT): $(SRC) src/amazon.h
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $(SRC)

data:
	python ../work/mkdata.py

clean:
	rm -f $(OUT) bin/amazon.wld

.PHONY: all clean data
