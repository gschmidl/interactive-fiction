# -*- coding: utf-8 -*-
"""Recover the source edits behind the Linux build's adv.text.

The Linux reference database was built from a proof-read copy of the cave
sources that was not archived: 31 of its 670 messages differ from the ones
this zip produces.  Every message in adv.text is just a run of consecutive
source lines sharing a 3-character marker, so each differing message can be
located in the cave files by its text and rewritten in place.
"""
import sys, os, glob, difflib

CAVE = sys.argv[1]          # directory holding the cave/adv_* sources
OURS = sys.argv[2]          # adv.text built from those sources
THEIRS = sys.argv[3]        # adv.text from the Linux build
OUT  = sys.argv[4]          # directory to write the revised sources into

def load(p): return open(p,'rb').read().decode('koi8-r')

# --- the line model getlin() uses ------------------------------------------
def lines_of(text):
    out=[]
    for raw in text.split('\n'):
        s=raw.expandtabs(8)
        s=(s+' '*81)[:81]
        skip = s[0]=='*' or (s[0]==' ' and s[3]==' ')
        out.append((raw, s, skip))
    return out

files = sorted(glob.glob(os.path.join(CAVE,'adv_*')))
model = {}          # file -> list of (raw, padded, skip)
index = []          # (file, lineno, marker, body) for every line getlin returns
for f in files:
    model[f]=lines_of(load(f))
    for i,(raw,s,skip) in enumerate(model[f]):
        if not skip:
            index.append((f,i,s[:3],s[3:].rstrip()))

ours   = open(OURS,'rb').read().split(b'\x00')
theirs = open(THEIRS,'rb').read().split(b'\x00')
assert len(ours)==len(theirs), (len(ours),len(theirs))

edits=[]           # (file, first_lineno, n_old_lines, [new_raw_lines])
unmatched=[]
for k,(a,b) in enumerate(zip(ours,theirs)):
    if a==b: continue
    old=a.decode('koi8-r').split('\n')
    new=b.decode('koi8-r').split('\n')
    while old and old[-1]=='': old.pop()
    while new and new[-1]=='': new.pop()
    hits=[]
    for p in range(len(index)-len(old)+1):
        run=index[p:p+len(old)]
        if all(run[j][3]==old[j] for j in range(len(old))) \
           and len({r[2] for r in run})==1 \
           and (p+len(old)==len(index) or index[p+len(old)][2]!=run[0][2]
                or index[p+len(old)][0]!=run[0][0]):
            hits.append(p)
    if len(hits)!=1:
        # fall back: unique match on the run alone, ignoring the boundary test
        hits=[p for p in range(len(index)-len(old)+1)
              if all(index[p+j][3]==old[j] for j in range(len(old)))
              and len({index[p+j][2] for j in range(len(old))})==1]
    if len(hits)!=1:
        unmatched.append((k,len(hits),old[0][:50])); continue
    p=hits[0]; f=index[p][0]; ln=index[p][1]; mk=index[p][2]
    # the source lines actually consumed may include skipped (blank) lines
    last=index[p+len(old)-1][1]
    edits.append((f, ln, last-ln+1, [mk+t for t in new]))

print("messages differing :", sum(1 for a,b in zip(ours,theirs) if a!=b))
print("located in sources :", len(edits))
for u in unmatched: print("  NOT LOCATED:", u)

# apply, back to front per file so line numbers stay valid
os.makedirs(OUT, exist_ok=True)
by_file={}
for f,ln,n,new in edits: by_file.setdefault(f,[]).append((ln,n,new))
for f in files:
    raw=[r for r,s,sk in model[f]]
    for ln,n,new in sorted(by_file.get(f,[]), reverse=True):
        raw[ln:ln+n]=new
    open(os.path.join(OUT,os.path.basename(f)),'wb').write(
        '\n'.join(raw).encode('koi8-r'))
print("wrote revised sources to", OUT)
