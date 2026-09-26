FC      = gfortran
# -fdefault-integer-8  a 36-bit word lives in a 64-bit integer
# -finit-local-zero    FORTRAN-10's loader zeroed core; the program relies
#                      on it (PLAC/FIXD are only set for listed objects)
# -ffixed-line-length-none  packed literals become long decimal constants
FFLAGS  = -std=legacy -fdec -fdefault-integer-8 -ffixed-line-length-none \
          -finit-local-zero -O2 -w
OBJ     = src/adv4ma.o src/adv4su.o src/runtime.o

bin/adv448.exe: $(OBJ) bin/FT01.DAT
	$(FC) -o $@ $(OBJ)

src/%.o: src/%.f
	$(FC) $(FFLAGS) -c -o $@ $<

src/adv4ma.f src/adv4su.f: ../src_its/ADV4MA.F4 ../src_its/ADV4SU.F4 \
                           tools/convert.py tools/patches.py
	cd .. && python port/tools/convert.py

bin/FT01.DAT: ../src_its/FT01.DAT
	cp $< $@

clean:
	rm -f src/*.o src/adv4ma.f src/adv4su.f bin/adv448.exe

.PHONY: clean
