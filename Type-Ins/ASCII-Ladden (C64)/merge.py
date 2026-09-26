import re, sys
prog={}
order=["part1.lst","part2.lst","part3.lst","part4.lst"]
overridden=[]
for f in order:
    for raw in open(f, encoding="utf-8"):
        raw=raw.rstrip("\n")
        if not raw.strip(): continue
        m=re.match(r"^(\d+)\s?(.*)$", raw)
        if not m: print("BAD LINE:",raw); sys.exit(1)
        n=int(m.group(1)); body=m.group(2)
        if n in prog: overridden.append((n,f))
        prog[n]=body
print("total lines:",len(prog))
print("overridden (later installment replaced placeholder):",overridden)
with open("ascii-ladden.lst","w",encoding="utf-8") as o:
    for n in sorted(prog): o.write(f"{n} {prog[n]}\n")
