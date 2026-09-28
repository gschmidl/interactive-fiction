r"""Make ADV751\VAR, the program's data file (travel table, short descriptions, object names and
the rest), the way Ruby did: by running his REFILL.BAS under QBasic.

    python make_var.py DOSBOX.EXE QBASIC.EXE [TREE]

DOSBOX.EXE is DOSBox 0.74, whose SDL 1.2 can run with no window (DOSBox Staging always opens
an OpenGL window); run it from a copy of its folder, as it writes stdout.txt beside itself.
QBASIC.EXE is MS-DOS QBasic 1.1.  Neither is part of this repository.  The run happens in
.build\ (not published), with no window and no sound, and the VAR it writes is copied into
ADV751\, or into TREE, another folder from make_tree.py (a --no-fixes one has REFILL.BAS as
sent, so its VAR keeps Ruby's object names).
"""
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
BUILD = os.path.join(HERE, ".build", "var")

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
C:\QBASIC.EXE /RUN REFILL.BAS
exit
"""


def main():
    dosbox, qbasic = (os.path.abspath(a) for a in sys.argv[1:3])
    tree = os.path.abspath(sys.argv[3]) if len(sys.argv) > 3 else os.path.join(HERE, "ADV751")
    c = os.path.join(BUILD, "c")
    if os.path.exists(BUILD):
        shutil.rmtree(BUILD)
    shutil.copytree(tree, os.path.join(c, "ADV751"))
    shutil.copyfile(qbasic, os.path.join(c, "QBASIC.EXE"))
    var = os.path.join(c, "ADV751", "VAR")
    if os.path.exists(var):
        os.remove(var)
    conf = os.path.join(BUILD, "refill.conf")
    with open(conf, "w", newline="\r\n") as f:
        f.write(CONF % c)
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    subprocess.run([dosbox, "-conf", conf, "-noconsole"], env=env, cwd=BUILD, timeout=300,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if not os.path.exists(var):
        sys.exit("REFILL.BAS wrote no VAR")
    shutil.copyfile(var, os.path.join(tree, "VAR"))
    print("VAR: %d bytes" % os.path.getsize(var))


if __name__ == "__main__":
    main()
