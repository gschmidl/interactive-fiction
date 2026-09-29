# add special needed modules here
#	BSD needs empty.c logdir.c
#	PWB needs ttyname.c
#	V6 needs ttyname.c logdir.c isatty.c
#       PDP11 profits from fsqrt.s
#       VAX needs fsqrtvax.c
ONEEDED = empty.o logdir.o fsqrtvax.o
SNEEDED = empty.c logdir.c fsqrtvax.c

CFLAGS = -O -I..

OBJECTS = $(ONEEDED) \
	announce.o anonmail.o arcpsltan.o arcptan.o atoi.o atoip.o \
	copy.o erlogf.o erlogm.o equal.o filesize.o fmod.o \
	getch.o getspd.o gtmod.o inflush.o \
	iceil.o instruct.o lock.o maxmin.o mesgoff.o modtime.o \
	muldiv.o myrlog.o myruid.o namuid.o notify.o parse.o \
	plur.o psin.o pslsin.o pswdsub.o pythag.o questn.o resetid.o \
	sendmsg.o sendtext.o setmodes.o sm.o snooze.o sread.o stmtch.o \
	sysinit.o uidnam.o vopen.o

SOURCES = $(SNEEDED) \
	announce.c anonmail.c arcpsltan.c arcptan.c atoi.c atoip.c \
	copy.c erlogf.c erlogm.c equal.c filesize.c fmod.c \
	getch.c getspd.c gtmod.c inflush.c \
	iceil.c instruct.c lock.c maxmin.c mesgoff.c modtime.c \
	muldiv.c myrlog.c myruid.c namuid.c notify.c parse.c \
	plur.c psin.c pslsin.c pswdsub.c pythag.c questn.c resetid.c \
	sendmsg.c sendtext.c setmodes.c sm.c snooze.c sread.c stmtch.c \
	sysinit.c uidnam.c vopen.c

GLIB = ../glib.a

all: $(GLIB)

sm.c:: ../sm_def.h ../crt.h
	touch $@

$(GLIB): $(SOURCES)
	-ar x $(GLIB)
	-$(CC) $(CFLAGS) -c $?
	rm -f $(GLIB)
	-ar ru $(GLIB) $(OBJECTS)
	rm -f *.o
	ranlib $(GLIB)

doc:
	imprint READ_ME
