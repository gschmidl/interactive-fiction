# if tty has no CLOSE_LINE use "OPT=-DNOCRTLDEL"
# if sys has no empty() call use "OPT=-DNOEMPTY"
OPT=
# V6 systems need "-lS", V7 systems need "-lm"
STDLIB = -lm
TCAPLIB=../libtcap.a
CFLAGS=-O
GLIB=../glib.a

# Remove helm if you have empty()
all: tcapmain log stardrek sd_fix snoop helm install

install: stardrek
	../install stardrek

tcapmain: sd_main.o sd_com.o tcapviz.o sd_sub.o sd_glb.o sd_sys.o $(GLIB)
	$(CC) sd_main.o sd_com.o tcapviz.o sd_sub.o sd_glb.o sd_sys.o \
	    $(GLIB) $(TCAPLIB) $(STDLIB) -o $@
	@echo "$@ is ready"

tcapviz.o: tcapviz.c sd_def.h
	$(CC) $(CFLAGS) $(OPT) -c tcapviz.c

sd_glb.o: sd_glb.c sd_def.h
	$(CC) $(CFLAGS) $(OPT) -c sd_glb.c

sd_sys.o: sd_sys.c sd_def.h
	$(CC) $(CFLAGS) $(OPT) -c sd_sys.c

log: log.o sd_glb.o $(GLIB)
	$(CC) $(CFLAGS) -n log.o sd_glb.o $(GLIB) $(STDLIB) -o $@
	@echo "$@ is ready"

stardrek: stardrek.o sd_glb.o
	$(CC) $(CFLAGS) stardrek.o sd_glb.o $(GLIB) -o $@
	chmod 2755 $@
	@echo "$@ is ready to be installed."

stardrek.o: stardrek.c sd_def.h
	$(CC) $(CFLAGS) -c stardrek.c

sd_fix: sd_fix.o sd_glb.o $(GLIB)
	$(CC) $(CFLAGS) $@.o sd_glb.o $(GLIB) $(STDLIB) -o $@
	@echo "$@ is ready"

snoop: snoop.o sd_glb.o $(GLIB)
	$(CC) $(CFLAGS) $@.o sd_glb.o $(GLIB) $(STDLIB) -o $@
	@echo "$@ is ready"

helm: helm.o sd_glb.o $(GLIB)
	$(CC) $(CFLAGS) helm.o sd_glb.o $(GLIB) $(STDLIB) -o $@
	chmod 700 $@
	@echo "$@ is installed"

sd_txt: sd_txt.nr
	nroff sd_txt.nr >sd_txt
