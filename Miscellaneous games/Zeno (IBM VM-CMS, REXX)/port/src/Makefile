CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -std=gnu99
OBJS    = main.o cms.o panel.o screen.o
TARGET  = ../Zeno.exe

all: $(TARGET)

$(TARGET): $(OBJS) libregina.a
	$(CC) -o $@ $(OBJS) libregina.a

%.o: %.c zeno.h
	$(CC) $(CFLAGS) -c -o $@ $<

harness.exe: harness.c panel.c zeno.h
	$(CC) $(CFLAGS) -o $@ harness.c panel.c

clean:
	rm -f $(OBJS) $(TARGET) harness.exe
