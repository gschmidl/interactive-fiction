r"""Play ADV751.BAS from a list of commands under real QBasic, with no window, and print what
the screen showed before each command.

    python smoke.py DOSBOX.EXE QBASIC.EXE COMMANDS.TXT [--tree ADV751-FOLDER] [--qb45 QB45]

--tree plays another build, e.g. one from make_tree.py --no-fixes (VAR comes from ../ADV751 if
it has none).  --qb45 plays the game compiled, as the collection runs it: the three programs
(with the changes below) are compiled with QuickBASIC 4.5 as ../make_exe.py does, and HELLO.EXE
is run instead of QBasic.  DOSBOX.EXE is DOSBox 0.74 (SDL 1.2 runs windowless; see ../make_var.py), QBASIC.EXE QBasic 1.1.
The run happens in ../.build/smoke (not published) and starts, like the collection's, with
QBASIC /RUN HELLO.BAS.  The copy of the game played there differs from ../ADV751 only so:
- HELLO.BAS answers its own question about the instructions with Y;
- every keyboard INPUT of ADV751.BAS takes the next line of COMMANDS.TXT instead (a game that
  runs out of commands ends there), and first writes the 80x25 text screen to LOG.TXT and
  clears it;
- a QBasic run-time error in ADV751.BAS is written to LOG.TXT with its line number, and ends
  the run;
- QUITS.BAS writes its screen to LOG.TXT before it leaves.
GAMES\ keeps what a SAVE wrote, so a second run can RESUME it: files already in
../.build/smoke/c/ADV751/GAMES are kept.

A line of COMMANDS.TXT that starts with # sets up a test instead of being typed: "#P 60" puts
the player in room 60, "#J 9 300" sets object 9's place (300 = carried, 400 = worn, 30n = in
container n), "#H 3" the number of things carried.  Follow a move with LOOK to see the room.
"""
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.join(HERE, "..")
RUN = os.path.join(PORT, ".build", "smoke")

DUMP = ('OPEN "C:\\LOG.TXT" FOR APPEND AS #3: DEF SEG = &HB800: '
        'FOR HR = 0 TO 24: HL$ = "": FOR HC = 0 TO 79: HL$ = HL$ + CHR$(PEEK((HR * 80 + HC) * 2)): '
        'NEXT HC: PRINT #3, RTRIM$(HL$): NEXT HR: DEF SEG')
HARNESS = [
    '60000 ' + DUMP,
    '60010 IF EOF(4) THEN PRINT #3, "=== out of commands": CLOSE #3: SYSTEM',
    '60020 LINE INPUT #4, HX$: PRINT #3, "=== > "; HX$: IF LEFT$(HX$, 1) = "#" THEN GOSUB 60100: GOTO 60010',
    '60030 CLOSE #3: CLS : PRINT HX$: RETURN',
    '60100 HS = INSTR(4, HX$ + " ", " "): HV = VAL(MID$(HX$, 4, HS - 4)): HW = VAL(MID$(HX$, HS + 1))',
    '60110 IF MID$(HX$, 2, 1) = "P" THEN P = HV',
    '60120 IF MID$(HX$, 2, 1) = "J" THEN J(HV) = HW',
    '60130 IF MID$(HX$, 2, 1) = "H" THEN H = HV',
    '60140 RETURN',
    '61000 OPEN "C:\\LOG.TXT" FOR APPEND AS #3: PRINT #3, "=== QBasic error"; ERR; "in line"; ERL: '
    'CLOSE #3: SYSTEM',
]

CONF = r"""[dosbox]
machine = svga_s3
memsize = 16
[cpu]
cycles = max
[mixer]
nosound = true
[autoexec]
mount c "%s"
c:
cd \ADV751
C:\QBASIC.EXE /RUN HELLO.BAS
exit
"""


def main():
    args = sys.argv[1:]
    tree = os.path.join(PORT, "ADV751")
    if "--tree" in args:                            # e.g. a make_tree.py --no-fixes build
        i = args.index("--tree")
        tree = os.path.abspath(args[i + 1])
        del args[i:i + 2]
    qb45 = None
    if "--qb45" in args:
        i = args.index("--qb45")
        qb45 = os.path.abspath(args[i + 1])
        del args[i:i + 2]
    dosbox, qbasic, commands = (os.path.abspath(a) for a in args[:3])
    c = os.path.join(RUN, "c")
    games = os.path.join(c, "ADV751", "GAMES")
    kept = {}
    if os.path.isdir(games):
        kept = {n: open(os.path.join(games, n), "rb").read() for n in os.listdir(games)}
    if os.path.exists(RUN):
        shutil.rmtree(RUN)
    shutil.copytree(tree, os.path.join(c, "ADV751"))
    if not os.path.exists(os.path.join(c, "ADV751", "VAR")):
        shutil.copyfile(os.path.join(PORT, "ADV751", "VAR"), os.path.join(c, "ADV751", "VAR"))
    for name, data in kept.items():
        open(os.path.join(games, name), "wb").write(data)
    shutil.copyfile(qbasic, os.path.join(c, "QBASIC.EXE"))
    lines = open(commands, "rb").read().replace(b"\r\n", b"\n").split(b"\n")
    open(os.path.join(c, "CMDS.TXT"), "wb").write(b"\r\n".join(lines).rstrip(b"\r\n") + b"\r\n")
    path = os.path.join(c, "ADV751", "HELLO.BAS")
    src = open(path, "rb").read()
    assert src.count(b'210 PRINT : INPUT "", D$') == 1
    open(path, "wb").write(src.replace(b'210 PRINT : INPUT "", D$', b'210 PRINT : D$ = "Y": PRINT D$'))

    path = os.path.join(c, "ADV751", "ADV751.BAS")
    src = open(path, "rb").read().decode("cp437")
    src, n = re.subn(r'\bINPUT\s+(?!#)(?:"[^"]*"\s*[,;]\s*)?([A-Z][A-Z0-9]*\$)',
                     r"GOSUB 60000: \1 = HX$", src)
    assert n == 14, n
    head = '12 C$ = CHR$(34)'
    assert src.count(head) == 1
    src = src.replace(head, head + ': OPEN "C:\\CMDS.TXT" FOR INPUT AS #4: ON ERROR GOTO 61000')
    src = src.rstrip("\r\n") + "\r\n" + "\r\n".join(HARNESS) + "\r\n"
    open(path, "wb").write(src.encode("cp437"))

    path = os.path.join(c, "ADV751", "QUITS.BAS")
    src = open(path, "rb").read().decode("cp437")
    assert src.count("5199 SYSTEM") == 1
    src = src.replace("5199 SYSTEM", "5199 " + DUMP + ": CLOSE #3: SYSTEM")
    open(path, "wb").write(src.encode("cp437"))

    conf_text = CONF % c
    if qb45:
        sys.path.insert(0, PORT)
        import make_exe
        make_exe.compile_all(dosbox, qb45, os.path.join(c, "ADV751"), os.path.join(PORT, ".build", "smoke-exe"))
        for name in make_exe.PROGRAMS:
            shutil.copyfile(os.path.join(PORT, ".build", "smoke-exe", "c", "SRC", name + ".EXE"),
                            os.path.join(c, "ADV751", name + ".EXE"))
        conf_text = conf_text.replace("C:\\QBASIC.EXE /RUN HELLO.BAS", "HELLO")
    conf = os.path.join(RUN, "smoke.conf")
    with open(conf, "w", newline="\r\n") as f:
        f.write(conf_text)
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    try:
        subprocess.run([dosbox, "-conf", conf, "-noconsole"], env=env, cwd=RUN, timeout=180,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except subprocess.TimeoutExpired:
        print("=== DOSBox did not finish (QBasic waiting in its editor or a dialog?)")
    log = os.path.join(c, "LOG.TXT")
    if not os.path.exists(log):
        sys.exit("no LOG.TXT")
    blank = 0
    for line in open(log, "rb").read().decode("cp437").split("\r\n"):
        blank = blank + 1 if not line else 0
        if blank < 2:
            print(line)


if __name__ == "__main__":
    main()
