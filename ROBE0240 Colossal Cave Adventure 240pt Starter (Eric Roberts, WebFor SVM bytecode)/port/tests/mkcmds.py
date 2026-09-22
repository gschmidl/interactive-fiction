"""Write a random command session (one command per line) and its JSON form for the browser."""
import json, random, sys
sys.argv += []
import importlib.util, os
spec = importlib.util.spec_from_file_location("fz", os.path.join(os.path.dirname(os.path.abspath(__file__)), "fuzz_words.py"))
fz = importlib.util.module_from_spec(spec); spec.loader.exec_module(fz)

seed = int(sys.argv[1]); n = int(sys.argv[2]); out = sys.argv[3]
r = random.Random(seed)
cmds = []
safe = ["n", "take all", "s", "w", "e", "d", "u", "in", "out", "inventory", "look", "score", "take lamp",
        "light lamp", "xyzzy", "plugh", "take food", "take bottle", "take keys", "unlock grate", "open grate",
        "take cage", "take rod", "take bird", "wave rod", "drop rod", "throw axe", "kill dwarf", "feed bear"]
for i in range(n):
    k = r.random()
    if k < 0.25:
        cmds.append(r.choice(fz.words))
    elif k < 0.55:
        cmds.append(r.choice(fz.words) + " " + r.choice(fz.words))
    elif k < 0.6:
        cmds.append(r.choice(["y", "n", "yes", "no", "?", "12", "help", "info"]))
    else:
        cmds.append(r.choice(safe))
open(out + ".txt", "w").write("\n".join(cmds) + "\n")
open(out + ".json", "w").write(json.dumps(cmds))
print(len(cmds))
