#!/usr/bin/env python3
"""
makefix.py - build ZENOFIX.EXEC from the author's original ZENO.EXEC.

Nine edits, making four changes, and nothing else.  All of them are
reproduced in ZENOFIX.diff with the reason.  Run from port/src:   python makefix.py
"""
import sys, os, difflib

HERE = os.path.dirname(os.path.abspath(__file__))
GAME = os.path.join(HERE, "..", "game")
SRC  = os.path.join(GAME, "ZENO.EXEC")
DST  = os.path.join(GAME, "ZENOFIX.EXEC")
DIFF = os.path.join(HERE, "..", "ZENOFIX.diff")

NOTES = """\
1. Survive a REXX error instead of ending the session.

   SIGNAL ON SYNTAX is armed once, at line 18, and never disarmed, so any
   interpreter error anywhere ended the whole game with "Due to your
   careless meddling...".  The commonest way to provoke one is a SIMPL/E
   expression that divides by zero: EVAL hands it straight to INTERPRET
   at line 1286, REXX raises error 42, and the session is over.

   Two changes make that survivable.  A RESUME: label goes in just after
   RESTART:, so the trap can re-enter the main loop with the world intact;
   and the handler cancels whatever program was executing, using the
   author's own abend path (CPMSG, DELPROG, and a TELL to the ERROR
   trapper), so the same faulty line cannot fire again on the next tick.
   RUNLINE sets ZRUN while it is running a program line, which is how the
   handler knows a program was to blame.  The handler also drops the clock
   entry that was firing: CLOCK removes it only after the call returns
   normally, so without this the same faulty line is re-dispatched on
   every tick and the game live-locks.

   The trap has to be re-armed by hand: SIGNAL ON is turned off while it
   is being handled.

2. EVAL: guard division by zero.

   The other guards in EVAL already return an expression unevaluated when
   it cannot be worked out; a zero divisor now does the same.  This keeps
   the commonest crash from reaching the trap at all.

3. EVAL: 'a' should be 'opa'.

   When either operand of a SIMPL/E comparison is not numeric, this line
   returns the letter A in place of the left operand, so that e.g.
   IF (ANS = KILL) compares 'A' against 'KILL'.  Every other return in
   the routine uses opa.  A plain typo.

4. SAVE: stop indenting the file by two columns every time.

   SAVEDATA wrote each record as `queue bl cr` with bl = ' ', which puts a
   blank and the separator blank in front of the data.  LOADDATA assigned
   the record straight back, so every save/restore cycle indented every
   program line and every book title by two more columns - after three
   round trips the LIFT program is six columns further right than it was.

   The prefix is dropped instead of compensated for in the reader, because
   ZENO INITDATA - which LOADDATA also reads - has no such prefix, and
   plenty of its lines start with real blanks that must be kept.  Save
   files now have exactly the shape of the shipped data file.  A save
   written by -original will still load two columns wide, and vice versa.
"""

# (exact original text, replacement)
FIXES = [

("""RESTART:
  call varinit
  do forever""",
 """RESTART:
  call varinit
RESUME:
  do forever"""),

("""RUNLINE:  parse arg pfn pvn pno
  if trace = @on then""",
 """RUNLINE:  parse arg pfn pvn pno
  zrun = 1
  if trace = @on then"""),

("""  else
    call addclock runtime 'RUNLINE' pfn pvn pnext
  return""",
 """  else
    call addclock runtime 'RUNLINE' pfn pvn pnext
  zrun = 0
  return"""),

("  if oper = '||' then return strip(opa||opb)",
 """  if oper = '||' then return strip(opa||opb)
  if oper = '/' & opb = 0 then return strip(opa oper opb)"""),

("  if badnum(opa opb) then return strip(a oper opb)",
 "  if badnum(opa opb) then return strip(opa oper opb)"),

("""  say "Due to your careless meddling, terrible things have happened"
  say "You have probably damaged things beyond repair!"
  say "The test is over - you have failed - begone!"
  say
  say "By the way, you might like to tell Dave Mitchell about this"
  say "Error" rc "occurred at line" sigl
  exit""",
 """  zerr = rc; zline = sigl
  signal on syntax
  if symbol('ZRUN') = 'VAR' then
    if zrun = 1 then
      do
        zrun = 0
        call cpmsg 'REXX error' zerr 'in' pfn pvn 'line' pno
        call cpmsg 'Abend 997 in' pfn pvn 'line' pno
        call delprog pfn pvn
        if pfn ^= 'ERROR' then
          call dotell '(ERROR)' 997 pfn pvn pno
      end
  if symbol('CLOCKNO') = 'VAR' then
    do
      zx = find(clocks,clockno)
      if zx ^= 0 then
        do
          clocks = delword(clocks,zx,1)
          drop clock.clockno
        end
    end
  commsg = 'Something went wrong (error' zerr 'at line' zline') - carrying on'
  actlines = ''
  if find('ROOM MTRANS COMPUTER',word(environment,1)) = 0 then
    environment = 'ROOM'
  signal RESUME"""),

("  queue bl disk",       "  queue disk"),
("        queue bl cr",   "        queue cr"),
("      queue bl book.cb.j", "      queue book.cb.j"),
("    queue bl cn note.cn", "    queue cn note.cn"),
]


def main():
    original = open(SRC, "rb").read().decode("latin-1")
    text = original
    for old, new in FIXES:
        old = old.replace("\n", "\r\n")
        new = new.replace("\n", "\r\n")
        n = text.count(old)
        if n != 1:
            sys.exit("makefix: %d matches for %r" % (n, old[:60]))
        text = text.replace(old, new)
    open(DST, "wb").write(text.encode("latin-1"))

    d = difflib.unified_diff(original.splitlines(), text.splitlines(),
                             "ZENO.EXEC", "ZENOFIX.EXEC", lineterm="", n=3)
    with open(DIFF, "w") as f:
        f.write("Changes made to Dave Mitchell's ZENO EXEC for the default,\n"
                "playable build.  Run Zeno.exe -original for the untouched one.\n"
                "Regenerate this file with src/makefix.py.\n\n")
        f.write(NOTES + "\n")
        f.write("\n".join(d) + "\n")
    print("wrote", os.path.normpath(DST))
    print("wrote", os.path.normpath(DIFF))


if __name__ == "__main__":
    main()
