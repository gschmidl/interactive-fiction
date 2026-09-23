FC      = gfortran
# -fdefault-integer-8       a 36-bit word lives in a 64-bit integer, and
#                           LOGICAL follows it, so a COMMON block is a
#                           flat array of words for the save file
# -fdec                     DEC bitwise .AND./.OR./.XOR. on integers
# -finit-local-zero         FORTRAN-10's loader zeroed core and the
#                           program relies on it
# -ffixed-line-length-none  packed literals become long decimal constants
FFLAGS  = -std=legacy -fdec -fdefault-integer-8 -ffixed-line-length-none \
          -finit-local-zero -O2 -w
OBJ     = src/advent.o src/runtime.o
DAT     = ../src_original/ADVENT.DAT

bin/advent350.exe: $(OBJ) bin/ADVENT.DAT
	@mkdir -p bin
	$(FC) -o $@ $(OBJ)

src/%.o: src/%.f
	$(FC) $(FFLAGS) -c -o $@ $<

src/advent.f: ../src_original/ADVENT.FOR tools/convert.py tools/patches.py
	python tools/convert.py

bin/ADVENT.DAT: $(DAT)
	@mkdir -p bin
	cp $< $@

# After building, check that no game routine was silently replaced by a
# libgfortran intrinsic of the same name -- RAN is the one that would
# have been (hence RAN10).  Everything the program calls must be defined
# in our own objects; the only names left undefined should be
# libgfortran's and libc's.
check-syms: $(OBJ)
	@echo "--- defined here ---"
	@nm $(OBJ) | grep " T " | awk '{print $$3}' | sort | tr '\n' ' '
	@echo ""
	@echo "--- wanted from elsewhere ---"
	@nm $(OBJ) | grep " U " | awk '{print $$2}' | sort -u \
	  | grep -v "^_gfortran" | tr '\n' ' '
	@echo ""

clean:
	rm -f src/*.o src/advent.f bin/advent350.exe bin/ADVENT.DAT

.PHONY: clean check-syms
