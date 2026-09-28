r"""Compile ADV751\HELLO.BAS, ADV751.BAS and QUITS.BAS into HELLO.EXE, ADV751.EXE and QUITS.EXE
with QuickBASIC 4.5, the way the collection runs the game.

    python make_exe.py DOSBOX.EXE QB45 [ADV751-FOLDER]

DOSBOX.EXE is DOSBox 0.74 (see make_var.py: its SDL 1.2 runs with no window).  QB45 is the
QuickBASIC 4.5 folder; BC.EXE, LINK.EXE and BCOM45.LIB are used, none of them part of this
repository.  Each program is compiled with /O (stand-alone: the run-time library is linked in,
no BRUN45.EXE needed) and /E (ADV751's SAVE and RESUME catch a bad file name with ON ERROR and
RESUME line), in .build\ (not published); the EXEs go into the folder (default ADV751 beside
this script).  The compiler's warnings are "Array not dimensioned": arrays Ruby used without
DIM, which QuickBASIC, like QBasic, gives 11 elements.
"""
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PROGRAMS = ("HELLO", "ADV751", "QUITS")

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
cd \SRC
%s
exit
"""


def compile_all(dosbox, qb45, folder, build):
    """Compile PROGRAMS from folder in build (a scratch folder); returns {name: log text}."""
    c = os.path.join(build, "c")
    if os.path.exists(build):
        shutil.rmtree(build)
    os.makedirs(os.path.join(c, "QB45"))
    os.makedirs(os.path.join(c, "SRC"))
    for name in ("BC.EXE", "LINK.EXE", "BCOM45.LIB"):
        shutil.copyfile(os.path.join(qb45, name), os.path.join(c, "QB45", name))
    lines = []
    for name in PROGRAMS:
        shutil.copyfile(os.path.join(folder, name + ".BAS"), os.path.join(c, "SRC", name + ".BAS"))
        lines.append(r"C:\QB45\BC %s.BAS,%s.OBJ,NUL /O /E; > %s.BC" % (name, name, name))
        lines.append(r"C:\QB45\LINK %s.OBJ,%s.EXE,NUL,C:\QB45\BCOM45.LIB; > %s.LNK" % (name, name, name))
    conf = os.path.join(build, "bc.conf")
    with open(conf, "w", newline="\r\n") as f:
        f.write(CONF % (c, "\n".join(lines)))
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    subprocess.run([dosbox, "-conf", conf, "-noconsole"], env=env, cwd=build, timeout=600,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    logs = {}
    for name in PROGRAMS:
        log = ""
        for ext in ("BC", "LNK"):
            path = os.path.join(c, "SRC", "%s.%s" % (name, ext))
            if os.path.exists(path):
                log += open(path, "rb").read().decode("cp437").replace("\r", "")
        logs[name] = log
        severe = re.search(r"(\d+) Severe\s+Error", log)
        if not severe or severe.group(1) != "0" or not os.path.exists(os.path.join(c, "SRC", name + ".EXE")):
            sys.exit("%s did not compile:\n%s" % (name, log))
    return logs


def main():
    dosbox, qb45 = (os.path.abspath(a) for a in sys.argv[1:3])
    folder = os.path.abspath(sys.argv[3] if len(sys.argv) > 3 else os.path.join(HERE, "ADV751"))
    build = os.path.join(HERE, ".build", "exe")
    logs = compile_all(dosbox, qb45, folder, build)
    for name in PROGRAMS:
        exe = os.path.join(build, "c", "SRC", name + ".EXE")
        shutil.copyfile(exe, os.path.join(folder, name + ".EXE"))
        warnings = re.search(r"(\d+) Warning Error", logs[name]).group(1)
        print("%s.EXE: %d bytes, %s warnings" % (name, os.path.getsize(exe), warnings))


if __name__ == "__main__":
    main()
