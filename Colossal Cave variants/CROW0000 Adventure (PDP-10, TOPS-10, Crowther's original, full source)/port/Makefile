FC      = gfortran
# -fdefault-integer-8       a 36-bit word lives in a 64-bit integer
# -fdec                     DEC bitwise .AND./.OR./.XOR. on integers
# -finit-local-zero         FORTRAN-10's loader zeroed core and the
#                           program relies on it (SETUP, QZ, GETIN's A(5))
# -ffixed-line-length-none  packed literals become long decimal constants
FFLAGS  = -std=legacy -fdec -fdefault-integer-8 -ffixed-line-length-none \
          -finit-local-zero -O2 -w
OBJ     = src/adv.o src/iofil.o src/runtime.o
DAT     = ../src_original/ADV.DAT

bin/adv.exe: $(OBJ) bin/ADV.DAT
	@mkdir -p bin
	$(FC) -o $@ $(OBJ)

src/%.o: src/%.f
	$(FC) $(FFLAGS) -c -o $@ $<

src/adv.f src/iofil.f: ../src_original/ADV.F4 ../src_original/IOFIL.FOR \
                       tools/convert.py tools/patches.py
	python tools/convert.py

src/adv.o: src/adv.f
src/iofil.o: src/iofil.f

bin/ADV.DAT: $(DAT)
	@mkdir -p bin
	cp $< $@

# Check that no routine of the program was silently replaced by a
# libgfortran intrinsic of the same name.  RAN is the one that would
# have been -- hence RAN10 -- and SHIFT is the one that was kept:
# Crowther's own SHIFT is 36-bit arithmetic that happens to be exactly
# right in 64-bit signed integers once the octal masks are
# sign-extended, so the port uses his, not a replacement.
check-syms: $(OBJ)
	@echo "--- defined here ---"
	@nm $(OBJ) | grep " T " | awk '{print $$3}' | sort | tr '\n' ' '
	@echo ""
	@echo "--- wanted from elsewhere ---"
	@nm $(OBJ) | grep " U " | awk '{print $$2}' | sort -u \
	  | grep -v "^_gfortran" | tr '\n' ' '
	@echo ""

clean:
	rm -f src/*.o src/adv.f src/iofil.f bin/adv.exe bin/ADV.DAT

.PHONY: clean check-syms
