CFLAGS = -O

all: qotd today install

install: qotd today
	../install qotd today

qotd: qotd.c
	$(CC) $(CFLAGS) -I/usr/include -I/usr/include/sys $@.c ../glib.a -o $@
	chmod 4755 $@
	@echo "$@ is ready"

today: today.c
	$(CC) $(CFLAGS) -I/usr/include -I/usr/include/sys $@.c ../glib.a -o $@
	chmod 4755 $@
	@echo "$@ is ready"
