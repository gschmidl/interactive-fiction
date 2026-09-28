r"""Make the eXo collection's copy of the game, in the layout its launch_if.bat starts with
DOSBox Staging (it mounts drives\c as C: and reads dosbox.conf):

    GAME\menu.txt
    GAME\MS-DOS\dosbox.conf
    GAME\MS-DOS\drives\c\ADV751\...     ADV751\ from make_tree.py, make_var.py, make_exe.py

The collection runs the compiled game (HELLO.EXE, ADV751.EXE, QUITS.EXE from make_exe.py), so
the .BAS files stay out: they go into the collection's Sources zip.  Only what the game reads
goes in: not REFILL.BAS (VAR is made already), and not the room files no line of the program
opens.

    python make_collection.py GAME
    python make_collection.py GAME --update

--update brings an existing copy up to date: it copies the game files that are new or have
changed, deletes nothing (saved games in GAMES\, LGHT and SCM stay), and lists what is there
but no longer part of the game.
"""
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
UNREAD = {"RMS/ALT38.TXT", "RMS/ALT157.TXT", "RMS/ROOM166.TXT", "RMS/ROOM167.TXT",
          "RMS/ROOM168.TXT", "RMS/ROOM179.TXT", "RMS/ROOM230.TXT"}
MADE_BY_GAME = {"LGHT", "SCM"}                   # written while it runs


def game_files(tree):
    """The tree's files that go into the collection, as (source path, path under ADV751)."""
    for root, dirs, files in os.walk(tree):
        rel = os.path.relpath(root, tree)
        for name in files:
            relname = os.path.normpath(os.path.join(rel, name)).replace(os.sep, "/")
            if relname in UNREAD or name.upper().endswith(".BAS"):
                continue
            yield os.path.join(root, name), relname


def update(game, tree):
    c = os.path.join(game, "MS-DOS", "drives", "c")
    adv = os.path.join(c, "ADV751")
    if not os.path.isdir(adv):
        sys.exit("%s is not a copy of the game" % game)
    wanted = set()
    for src, rel in game_files(tree):
        wanted.add(rel)
        dst = os.path.join(adv, *rel.split("/"))
        if not os.path.exists(dst) or open(src, "rb").read() != open(dst, "rb").read():
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            print("%s ADV751/%s" % ("changed" if os.path.exists(dst) else "new    ", rel))
            shutil.copy2(src, dst)
    conf = os.path.join(game, "MS-DOS", "dosbox.conf")
    if open(conf, "rb").read() != open(os.path.join(HERE, "dosbox.conf"), "rb").read():
        print("changed MS-DOS/dosbox.conf")
        shutil.copyfile(os.path.join(HERE, "dosbox.conf"), conf)
    for root, dirs, files in os.walk(c):
        for name in files:
            rel = os.path.relpath(os.path.join(root, name), adv).replace(os.sep, "/")
            if rel not in wanted and not rel.startswith("GAMES/") and rel not in MADE_BY_GAME:
                print("not part of the game any more:", os.path.relpath(os.path.join(root, name), game))


def main():
    tree = os.path.join(HERE, "ADV751")
    for need in ("VAR", "HELLO.EXE", "ADV751.EXE", "QUITS.EXE"):
        if not os.path.isfile(os.path.join(tree, need)):
            sys.exit("no ADV751\\%s: run make_var.py and make_exe.py first" % need)
    game = os.path.abspath(sys.argv[1])
    if sys.argv[2:3] == ["--update"]:
        return update(game, tree)
    if os.path.exists(game):
        sys.exit("%s exists already" % game)
    adv = os.path.join(game, "MS-DOS", "drives", "c", "ADV751")
    os.makedirs(os.path.join(adv, "GAMES"))       # SAVE writes GAMES\NAME
    open(os.path.join(game, "menu.txt"), "wb").close()
    shutil.copyfile(os.path.join(HERE, "dosbox.conf"), os.path.join(game, "MS-DOS", "dosbox.conf"))
    count = 0
    for src, rel in game_files(tree):
        dst = os.path.join(adv, *rel.split("/"))
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copy2(src, dst)
        count += 1
    print("%s: %d game files" % (game, count))


if __name__ == "__main__":
    main()
