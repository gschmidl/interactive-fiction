CC     = gcc
CFLAGS = -O2 -Wall -Wextra -std=c99

aosvs32.exe: src32/mv32.c src32/net.c src32/net.h src32/host.h src32/d200.h src32/sched.h src32/qdefs.h src32/mv.h src32/wide.h src32/wideexec.h src32/mvops.h src32/syscall32.h src32/quest.h src32/mvfpu.h
	$(CC) $(CFLAGS) -o $@ src32/mv32.c src32/net.c -lws2_32

clean:
	rm -f aosvs32.exe

.PHONY: clean
