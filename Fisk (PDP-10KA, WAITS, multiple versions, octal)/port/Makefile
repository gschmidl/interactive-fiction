# FisK - PDP-10/WAITS core image interpreter
CC      = gcc
CFLAGS  = -O2 -Wall
LDLIBS  = -lm
SRC     = src/cpu.c src/image.c src/main.c src/waits.c
BIN     = bin/fisk.exe

DUMPS   = jfp=../dump_original/rjb.jfp/fisk.dmp/775,../dump_original/rjb.jfp/fisk.txt/637 \
          v31=../dump_original/3.1/fisk.dmp/775,../dump_original/rjb.1/fisk.txt/551

$(BIN): $(SRC) src/fisk.h
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDLIBS)

# only needed if the octal transfer files change
image:
	python tools/mkimage.py src/image.c $(DUMPS)

clean:
	rm -f $(BIN)

.PHONY: image clean
