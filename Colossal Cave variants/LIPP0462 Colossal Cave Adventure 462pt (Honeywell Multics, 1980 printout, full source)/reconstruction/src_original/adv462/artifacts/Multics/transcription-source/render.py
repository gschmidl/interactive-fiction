#!/usr/bin/env python3
"""Render adventure.src (Woods-style, TAB-separated) into the printout's fixed-width layout.
First field right-justified in 8 columns (Fortran i8, per format statements 1003/1005/1031/1041 in adventure_.fortran); text follows immediately.
Numeric fields: 8 columns each, right-justified.
A field of the form '@<col><text>' places <text> starting at 1-based column <col>."""
import re
FIELD1=8
num=re.compile(r'-?\d+$')
out=[]
for l in open('adventure.src').read().splitlines():
    f=l.split('\t')
    s=f[0].rjust(FIELD1)
    rest=f[1:]
    if rest and all(num.match(x) for x in rest):
        out.append(s+''.join(x.rjust(8) for x in rest)); continue
    for j,x in enumerate(rest):
        m=re.match(r'@(\d+)(.*)',x)
        if m: s=s.ljust(int(m.group(1))-1)+m.group(2)
        elif num.match(x) and j<len(rest)-1: s+=x.rjust(8)
        else: s+=x
    out.append(s)
open('adventure.data','w').write('\n'.join(out)+'\n')
