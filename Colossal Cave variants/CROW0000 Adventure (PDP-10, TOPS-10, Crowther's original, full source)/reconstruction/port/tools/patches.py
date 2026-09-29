#!/usr/bin/env python3
"""patches.py -- the exact-line replacements convert.py applies to
../src_original/ADV.F4 and ../src_original/IOFIL.FOR.

Each entry quotes the original line verbatim, tabs and all.  convert.py
fails the build if a quoted line is not found, so a patch cannot quietly
stop applying.  Replacement lines are already in fixed-form columns and
bypass the mechanical rules.

Everything here is I/O or startup.  No game logic is patched: the only
line that changes what the program decides is the one that makes
PAUSE 'INIT DONE' conditional, and that reproduces the SAVE the
distribution's own instructions tell you to do.
"""

# ----------------------------------------------------------------------
# ADV.F4
# ----------------------------------------------------------------------

ADV = [

# --- declarations -----------------------------------------------------
("\tREAL RAN",
 ["      DOUBLE PRECISION RAN10",
  "      DIMENSION GBUF(12)",
  "      CHARACTER*100 OUTBF(4)",
  "      CHARACTER*5 T5A, T5B"],
 "RAN is a gfortran intrinsic, so the program would have been silently "
 "linked against the library's generator instead of FORLIB's; renamed "
 "RAN10 and declared double, because the port carries the PDP-10's "
 "27-bit float values exactly and compares them against constants "
 "folded the same way.  GBUF/OUTBF/T5A/T5B are the buffers the free-"
 "format read and the A-descriptor writes below need."),

("\tIF(SETUP.NE.0) GOTO 1",
 ["      CALL BOOT",
  "      IF(SETUP.NE.0) GOTO 1"],
 "The command line is read before anything else.  The test itself is "
 "untouched: SETUP is the saved-core-image check, and since there is no "
 "core image on Windows it is always 0 and the branch is never taken, "
 "exactly as on a fresh RUN of an unsaved image."),

# --- reading the database ---------------------------------------------
("1002\tREAD(1,1003) IKIND",
 ["1002  CALL RDFRE(1, GBUF, 1)",
  "      IKIND = GBUF(1)"],
 "FORMAT(G): DEC free-format integer input."),

("1003\tFORMAT(G)",
 [],
 "Dead with the read above gone, and gfortran rejects a G descriptor "
 "with no field width."),

("1004\tREAD(1,1005)JKIND,(LLINE(I,J),J=3,22)",
 ["1004  CALL RDMSG(1, JKIND, LLINE, 1000, I, 3, 22)"],
 "FORMAT(1G,20A5): a free-format integer, then twenty packed five-"
 "character words.  Where the A fields start after the number is the "
 "whole of how this program prints -- see GSKIP in runtime.f."),

("1005\tFORMAT(1G,20A5)",
 [],
 "Dead with the read above gone; gfortran rejects the width-less G."),

("1014\tREAD(1,1015)JKIND,LKIND,(TK(L),L=1,10)",
 ["1014  CALL RDFRE(1, GBUF, 12)",
  "      JKIND = GBUF(1)",
  "      LKIND = GBUF(2)",
  "      DO L = 1, 10",
  "         TK(L) = GBUF(L+2)",
  "      END DO"],
 "FORMAT(12G): twelve free-format integers from one record, the rest "
 "left zero.  The program depends on that zero fill -- a travel record "
 "carries a variable number of motion verbs and the loop at 1018 stops "
 "at the first zero."),

("1015\tFORMAT(12G)",
 [],
 "Dead with the read above gone; gfortran rejects the width-less G."),

("\tREAD(1,1021) KTAB(IU),ATAB(IU)",
 ["      CALL RDVOC(1, KTAB(IU), ATAB(IU))"],
 "FORMAT(G,A5): a free-format integer and one packed word."),

("1021\tFORMAT(G,A5)",
 [],
 "Dead with the read above gone; gfortran rejects the width-less G."),

# --- output -----------------------------------------------------------
("\tTYPE 67,DTOT",
 ["      WRITE(OUTBF,67) DTOT",
  "      CALL PUTRCS(OUTBF, 2)"],
 "FORMAT 67 is left exactly as written, including the string that runs "
 "across a continuation line, and gfortran's own format processor makes "
 "the two records the trailing slash asks for; PUTRCS then applies "
 "FORTRAN carriage control, which gfortran cannot be told to do on "
 "unit 6."),

("\tTYPE 78,ATTACK",
 ["      WRITE(OUTBF,78) ATTACK",
  "      CALL PUTRCS(OUTBF, 2)"],
 "As TYPE 67."),

("\tTYPE 68,STICK",
 ["      WRITE(OUTBF,68) STICK",
  "      CALL PUTRCS(OUTBF, 2)"],
 "As TYPE 67."),

("4\tTYPE 5,(LLINE(KK,JJ),JJ=3,LLINE(KK,2))",
 ["4     CALL PUTLL(LLINE, 1000, KK, 3, LLINE(KK,2))"],
 "FORMAT(20A5) over packed words: the A descriptor on a 36-bit word of "
 "five seven-bit characters, which gfortran cannot do.  The record is "
 "five characters per word with the trailing blanks the original "
 "printed."),

("\tTYPE 6",
 ["      WRITE(OUTBF,6)",
  "      CALL PUTRCS(OUTBF, 2)"],
 "FORMAT(/) with an empty list is two empty records; gfortran produces "
 "both, PUTRCS turns each into one new line as FOROTS did."),

("2005\tTYPE 2006,(LLINE(KK,JJ),JJ=3,LLINE(KK,2))",
 ["2005  CALL PUTLL(LLINE, 1000, KK, 3, LLINE(KK,2))"],
 "As TYPE 5."),

("\tTYPE 2007",
 ["      WRITE(OUTBF,2007)",
  "      CALL PUTRCS(OUTBF, 2)"],
 "As TYPE 6."),

("\tTYPE 5063,A",
 ["      CALL W2C(A, 5, T5A)",
  "      WRITE(OUTBF,5063) T5A",
  "      CALL PUTRCS(OUTBF, 2)"],
 "One A5 item is a packed word; unpacked into five characters so that "
 "the format, which is unchanged, prints exactly what it printed on the "
 "-10."),

("5333\tTYPE 5334,A,B",
 ["5333  CALL W2C(A, 5, T5A)",
  "      CALL W2C(B, 5, T5B)",
  "      WRITE(OUTBF,5334) T5A, T5B",
  "      CALL PUTRCS(OUTBF, 2)"],
 "As TYPE 5063, with two words."),

("\tTYPE 5005,A",
 ["      CALL W2C(A, 5, T5A)",
  "      WRITE(OUTBF,5005) T5A",
  "      CALL PUTRCS(OUTBF, 2)"],
 "As TYPE 5063."),

("5316\tTYPE 5317,A,B",
 ["5316  CALL W2C(A, 5, T5A)",
  "      CALL W2C(B, 5, T5B)",
  "      WRITE(OUTBF,5317) T5A, T5B",
  "      CALL PUTRCS(OUTBF, 2)"],
 "As TYPE 5334."),

("\tTYPE 5001,A",
 ["      CALL W2C(A, 5, T5A)",
  "      WRITE(OUTBF,5001) T5A",
  "      CALL PUTRCS(OUTBF, 2)"],
 "As TYPE 5063."),

("5314\tTYPE 5315,A,B",
 ["5314  CALL W2C(A, 5, T5A)",
  "      CALL W2C(B, 5, T5B)",
  "      WRITE(OUTBF,5315) T5A, T5B",
  "      CALL PUTRCS(OUTBF, 2)"],
 "As TYPE 5334."),

# --- terminal input ---------------------------------------------------
("6\tACCEPT 1,(A(I), I=1,4)",
 ["6     CALL RDA5(A, 4)"],
 "ACCEPT with FORMAT(4A5): one terminal record packed into four words, "
 "left-justified and blank-filled.  FORMAT 1 is left in place as the "
 "record's own description."),

# --- the initialisation pause ----------------------------------------
("\tPAUSE 'INIT DONE'",
 ["      IF (OPTINI() .NE. 0) CALL PAUSEM('INIT DONE')"],
 "This PAUSE ends the one-off first run: the -10 procedure is LOAD, "
 "RUN, then SAVE the initialised core image, and the saved image skips "
 "straight past it because SETUP is no longer 0.  The pack shipped "
 "ADV.EXE *unsaved* -- 48 blocks, the same size as a fresh LOAD -- so "
 "RUN ADV really does re-read ADV.DAT and stop here every time.  The "
 "port has no core image either, so it reads the database at every "
 "start; by default it then carries straight on, as the saved image "
 "would, and -i reproduces the shipped one."),

# --- SPEAK ------------------------------------------------------------
("\tDIMENSION RTEXT(100),LLINE(1000,22)",
 ["      DIMENSION RTEXT(100),LLINE(1000,22)",
  "      CHARACTER*100 OUTBF(4)"],
 "SPEAK needs the same record buffer for its TYPE 996."),

("999\tTYPE 998, (LLINE(KKT,JJT),JJT=3,LLINE(KKT,2))",
 ["999   CALL PUTLL(LLINE, 1000, KKT, 3, LLINE(KKT,2))"],
 "As TYPE 5."),

("997\tTYPE 996",
 ["997   WRITE(OUTBF,996)",
  "      CALL PUTRCS(OUTBF, 2)"],
 "As TYPE 6."),
]

# ----------------------------------------------------------------------
# IOFIL.FOR -- Paul T. Robinson's 1980 replacements for the library's
# IFIL/OFIL.  ADV calls IFILE; OFILE is never called but is converted
# too, because it is part of the recovered source.
# ----------------------------------------------------------------------

IOFIL = [

("\tSUBROUTINE IFILE(IUNIT, NAME, IEXT)",
 ["      SUBROUTINE IFILE(IUNIT, NAME, IEXT)",
  "      IMPLICIT INTEGER(A-Z)"],
 "DNAME was DOUBLE PRECISION only to give ENCODE two words to write "
 "into; as two integer words it needs the implicit typing to match the "
 "rest of the program."),

("\tSUBROUTINE OFILE(IUNIT,NAME,IEXT)",
 ["      SUBROUTINE OFILE(IUNIT,NAME,IEXT)",
  "      IMPLICIT INTEGER(A-Z)"],
 "As IFILE."),

("\tDOUBLE PRECISION DNAME",
 ["      DIMENSION DNAME(2)"],
 "Two 36-bit words, which is what a DOUBLE PRECISION was being used as "
 "here."),

("\tENCODE(10,10,DNAME) NAME, IEXT",
 ["      DNAME(1) = NAME",
  "      DNAME(2) = IEXT"],
 "ENCODE with FORMAT(2A5) into two words is exactly a copy of the two "
 "packed arguments -- 'ADV  ' and '.DAT '."),

("10\tFORMAT(2A5)",
 [],
 "Dead with the ENCODE gone."),

("\tOPEN(UNIT=IUNIT,FILE=DNAME,ACCESS='SEQIN')",
 ["      CALL OPNSEQ(IUNIT, DNAME, 2, 1)"],
 "DEC's OPEN took the file name as packed words and ACCESS='SEQIN' for "
 "read-only sequential; OPNSEQ unpacks the name, drops the blanks that "
 "are not part of a TOPS-10 name, and looks beside the executable as "
 "well as in the working directory."),

("\tOPEN(UNIT=IUNIT, FILE=DNAME, ACCESS='SEQOUT')",
 ["      CALL OPNSEQ(IUNIT, DNAME, 2, 2)"],
 "As IFILE's, for writing."),
]
