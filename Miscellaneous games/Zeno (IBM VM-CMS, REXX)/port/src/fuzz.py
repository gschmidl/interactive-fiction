import random, subprocess, os, sys, re
EXE = r"Zeno.exe"
objs = "ALIE NOTE SUIT BLAS SHUT LASE WIND DESK ROBO BOOK LEAF PAPE BUTT COMP TERM LIFT ROOM DOOR BOTT STAI HATC UP DOWN NEAR SHIE GLUR".split()
verbs = "go take drop read press open close push pull look examine enter use wear remove kill hit throw".split()
comp  = ["list","dir","active","clear","type help","type lift","e lift","e newprog","air","alert",
         "robot","terminal","status","game","cancel lift","erase junk","disk advanced","disk empty",
         "tell lift hello","run lift","help","?","list ?","dir ?"]
ecmds = ["","top","bot","up","down","up 5","down 5","f lift","find xyzzy","i newline","d","d 3",
         "c/a/b/","x","quit","file","save","help","%%%","1","-1","99999"]
keys  = ["k enter"]*6 + ["k pf1","k pf2","k pf3","k pf4","k pf5","k pf6","k pf7","k pf8","k pf9","k pf10","k pf11","k pf12"]

def gen(n, mode):
    L=[]
    for _ in range(n):
        r = random.random()
        if mode=="room":
            if r<0.6: L.append("t "+random.choice(verbs)+" "+random.choice(objs).lower())
            elif r<0.7: L.append("t "+random.choice(comp))
        elif mode=="comp":
            if r<0.7: L.append("t "+random.choice(comp))
        else:
            if r<0.5: L.append("t "+random.choice(ecmds))
            elif r<0.6: L.append("f %d %s" % (random.randint(1,24), random.choice(["","aa","i","d","1","c99"])))
        L.append(random.choice(keys))
    L.append("k pf3"); L.append("k pf3"); L.append("k pf3")
    return "\n".join(L)+"\n"

bad = re.compile(r"careless meddling|Error \d+ occurred|gone wrong trying")
fails=0
for i in range(int(sys.argv[1])):
    mode = random.choice(["room","room","comp","edit"])
    pre = ""
    if mode=="comp": pre = "t go to computer\nk enter\n"
    if mode=="edit": pre = "t go to computer\nk enter\nt e lift\nk enter\n"
    s = pre + gen(28, mode)
    open("./fuzzwork/fz.txt","w").write(s)
    try:
        out = subprocess.run([EXE,"-script","./fuzzwork/fz.txt","-fast"],capture_output=True,
                             text=True,timeout=45).stdout
    except subprocess.TimeoutExpired:
        print("=== HANG  seed",i,"mode",mode); open("./fuzzwork/hang%d.txt"%i,"w").write(s); fails+=1; continue
    m = bad.search(out)
    if m:
        fails+=1
        print("=== CRASH seed",i,"mode",mode)
        for ln in out.splitlines():
            if bad.search(ln) or "Error" in ln: print("   ",ln)
        open("./fuzzwork/crash%d.txt"%i,"w").write(s)
        open("./fuzzwork/crashout%d.txt"%i,"w").write(out)
print("runs=%s failures=%d" % (sys.argv[1], fails))
