import sys
def m(s):
    s=(s+' '*32)[:32]
    return ''.join('.' if c==' ' else '#' for c in s)
key=sys.argv[1]; f=sys.argv[2]
masks=[]; cur=None
for line in open('masks.txt'):
    if line.startswith('== '):
        cur=line.split()[1]; continue
    if cur==key: masks.append(line.rstrip('\n')[4:])
rows=[l for l in open(f).read().split('\n')]
if rows and rows[-1]=='': rows.pop()
print(f"transcribed {len(rows)} rows, mask rows {len(masks)}")
bad=0
for i,r in enumerate(rows):
    if i>=len(masks):
        print(f"{i:3d} NO MASK  {r!r}"); break
    a=m(r); b=masks[i]
    if a!=b:
        diff=''.join('^' if x!=y else ' ' for x,y in zip(a,b))
        miss=''.join('M' if x=='#' and y=='.' else ' ' for x,y in zip(a,b))
        tag=''
        if miss.strip(): tag+='  MISSING-INK'
        if a.rfind('#')!=b.rfind('#'): tag+='  TAILDIFF'
        print(f"{i:3d} txt |{a}|")
        print(f"    msk |{b}|")
        print(f"    dif |{diff}|{tag}  {r!r}")
        bad+=1
print("mismatches",bad)
