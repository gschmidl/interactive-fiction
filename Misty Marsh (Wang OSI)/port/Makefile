# Misty Marsh -- build the interpreter from the disk image.
#
#   make            build misty.exe from the data.h already in src/
#   make regen      redo the whole chain: disk image -> data.h -> misty.exe
#   make clean

# ?= would lose to make's own built-in CC=cc, which is not here.
ifeq ($(origin CC),default)
CC = gcc
endif
CFLAGS  ?= -O2 -Wall
PYTHON  ?= python
IMG     ?= ../src_original/mistymarsh.img

misty.exe: src/misty.c src/data.h
	$(CC) $(CFLAGS) -o $@ src/misty.c

# The glossary comes off the disk every time; nothing here is hand-written.
regen:
	cd tools && $(PYTHON) extract.py ../$(IMG) ../data
	cd tools && $(PYTHON) parse.py
	cd tools && $(PYTHON) gendata.py
	$(MAKE) misty.exe

clean:
	rm -f misty.exe

.PHONY: regen clean
