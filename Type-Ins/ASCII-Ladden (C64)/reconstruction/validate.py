import re
lines={}
for raw in open("ascii-ladden.lst",encoding="utf-8"):
    n,_,b=raw.rstrip("\n").partition(" ")
    lines[int(n)]=b
defined=set(lines)

def strip_quotes(s):
    out=[];inq=False
    for ch in s:
        if ch=='"': inq=not inq; out.append(' ')
        else: out.append(' ' if inq else ch)
    return "".join(out)

bad=[]; refs=0
for n,b in sorted(lines.items()):
    code=strip_quotes(b)
    # GOTO / GOSUB / THEN followed by number(s); also ON..GOTO lists
    for m in re.finditer(r"(GOTO|GOSUB|THEN|RUN)\s*([0-9][0-9,\s]*)", code):
        kw=m.group(1); nums=re.findall(r"\d+", m.group(2))
        for t in nums:
            refs+=1
            if int(t) not in defined: bad.append((n,kw,int(t)))
print(f"{len(defined)} lines, {refs} branch references checked")
if bad:
    print("UNDEFINED TARGETS:")
    for n,kw,t in bad: print(f"  line {n}: {kw} {t}")
else: print("all branch targets resolve")
# line-number ordering / duplicates
ns=sorted(defined); print("range", ns[0], "-", ns[-1])
