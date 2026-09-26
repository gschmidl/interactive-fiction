"""Generate BASIC/3000 loader scripts that build the game's four data files.

The three text files are fed through LINPUT, so nothing in the text has to be
escaped.  The movement table is written from BASIC/3000 character constants
('n is a one-character string with code n), which is the only way to get the
NUL and 255 bytes the table needs into a record.

usage: make_loaders.py [play|print]     (default: play = with fixes applied)
"""
import os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
T = os.path.normpath(os.path.join(HERE, "..", "transcription"))

# records to CREATE: generous, the file's logical end is where we stop writing
RECS = {"AITEMS": 40, "ADESCRIP": 80, "AMESSAGE": 200, "AMOVING": 30}


def text_loader(name, lines, recs):
    out = ["PURGE %s" % name, "#BASIC"]
    out += [
        "5 FILES *",
        "10 DIM A$[100]",
        '20 CREATE N,"%s",%d' % (name, recs),
        '30 IF N=0 THEN 50',
        '40 PRINT "CREATE FAILED, RC=";N',
        '50 ASSIGN "%s",1,R' % name,
        '60 IF R=0 THEN 80',
        '70 PRINT "ASSIGN FAILED, RC=";R',
        "80 C=0",
        '85 PRINT "@@";',
        "90 LINPUT A$",
        '100 IF A$="*EOF*" THEN 140',
        "110 PRINT #1;A$",
        "120 C=C+1",
        "130 GOTO 85",
        "140 ASSIGN *,1",
        '150 PRINT "WROTE";C;"RECORDS TO %s"' % name,
        "160 END",
        "RUN",
    ]
    out += ["#FEED " + l for l in lines]
    out += ["#FEED *EOF*", "#WAITPROMPT", "SCRATCH", "#MPE"]
    return out


def moving_loader(rows, recs):
    out = ["PURGE AMOVING", "#BASIC"]
    out += [
        "5 FILES *",
        "10 DIM A$[10]",
        '20 CREATE N,"AMOVING",%d' % recs,
        '30 IF N=0 THEN 50',
        '40 PRINT "CREATE FAILED, RC=";N',
        '50 ASSIGN "AMOVING",1,R' ,
        '60 IF R=0 THEN 110',
        '70 PRINT "ASSIGN FAILED, RC=";R',
        '80 STOP',
    ]
    n = 100
    for row in rows:
        n += 10
        lit = "".join("'%d" % int(v) for v in row)
        out.append("%d PRINT #1;%s" % (n, lit))
    out.append("%d ASSIGN *,1" % (n + 10))
    out.append('%d PRINT "WROTE %d ROOMS TO AMOVING"' % (n + 20, len(rows)))
    out.append("%d END" % (n + 30))
    out += ["RUN", "SCRATCH", "#MPE"]
    return out


def main(which="play"):
    src = os.path.join(T, "data_play" if which == "play" else "data_print")
    scripts = {}
    for name in ("AITEMS", "ADESCRIP", "AMESSAGE"):
        lines = open(os.path.join(src, name), encoding="ascii").read().split("\n")
        if lines and lines[-1] == "":
            lines.pop()
        scripts[name] = text_loader(name, lines, RECS[name])
    rows = [l.split() for l in open(os.path.join(src, "AMOVING.txt"), encoding="ascii") if l.strip()]
    scripts["AMOVING"] = moving_loader(rows, RECS["AMOVING"])
    for name, lines in scripts.items():
        path = os.path.join(HERE, "load_%s.txt" % name)
        open(path, "w", encoding="ascii", newline="\n").write("\n".join(lines) + "\n")
        print("%-10s %4d script lines -> %s" % (name, len(lines), os.path.basename(path)))


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "play")
