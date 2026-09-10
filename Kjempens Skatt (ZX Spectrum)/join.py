import re, sys
order=['p1c1','p1c2','p2c1','p2c2','p3c1','p4c1','p4c2','p5c1','p5c2','p6c2']
rows=[]
for k in order:
    rs=[l for l in open(f'rows_{k}.txt').read().split('\n')]
    if rs and rs[-1]=='': rs.pop()
    rows += rs
print(f"total rows {len(rows)}", file=sys.stderr)
pat=re.compile(r'^( {0,3}\d{1,4}) ')
lines=[]; cur=None; last=-1
for r in rows:
    m=pat.match(r)
    isnew=False
    if m and len(m.group(1))==4:
        n=int(m.group(1))
        if n>last: isnew=True
    if isnew:
        if cur is not None: lines.append(cur)
        last=int(m.group(1)); cur=[last, [r[5:]]]
    else:
        if cur is None: raise SystemExit("orphan row: "+repr(r))
        cur[1].append(r)
lines.append(cur)
out=[]
for num,parts in lines:
    padded=[]
    for i,p in enumerate(parts[:-1]):
        w=27 if i==0 else 32
        padded.append((p+' '*w)[:w])
    txt=''.join(padded)+parts[-1]
    out.append((num,txt))
print(f"lines {len(out)}", file=sys.stderr)
with open('listing.txt','w') as f:
    for n,t in out: f.write(f"{n} {t}\n")
