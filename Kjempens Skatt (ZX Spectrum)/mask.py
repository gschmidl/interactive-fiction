import fitz, numpy as np, sys
from PIL import Image
import os
# The magazine scan is not redistributed with this repository.
# Set KJEMPENS_PDF to point at it.
PDF = os.environ.get('KJEMPENS_PDF', 'Kjempens skatt (Norwegian).pdf')
doc = fitz.open(PDF)
DPI=300
_cache={}
def page_arr(pg):
    if pg not in _cache:
        pm = doc[pg-1].get_pixmap(dpi=DPI, colorspace=fitz.csGRAY)
        a = np.frombuffer(pm.samples, dtype=np.uint8).reshape(pm.height, pm.stride)[:, :pm.width]
        _cache[pg]=a
    return _cache[pg]

def region(pg,x0,x1,y0,y1,thresh=170):
    a=page_arr(pg); H,W=a.shape
    sub=a[int(y0*H):int(y1*H), int(x0*W):int(x1*W)]
    return (sub<thresh).astype(np.int32)

def fit_grid(ink):
    P=ink.sum(axis=0).astype(float)
    W=len(P)
    best=None
    for pitch in np.arange(W/33.5, W/30.5, 0.02):
        for off in np.arange(-pitch, pitch, 0.5):
            xs=off+pitch*np.arange(33)
            xs=xs[(xs>=0)&(xs<W-1)]
            if len(xs)<25: continue
            idx=xs.astype(int)
            s=P[idx].sum()/len(idx)
            if best is None or s<best[0]: best=(s,off,pitch)
    return best[1],best[2]

def rows(ink, minrun=3):
    r=ink.sum(axis=1); on=r>2
    bands=[];i=0
    while i<len(on):
        if on[i]:
            j=i
            while j<len(on) and on[j]: j+=1
            bands.append((i,j)); i=j
        else: i+=1
    return bands

def cells(pg,x0,x1,y0,y1,off=None,pitch=None,rx0=None,rx1=None,ry0=None,ry1=None):
    # off/pitch fitted on the big region (rx*, ry*) then applied to row (y0,y1)
    if off is None:
        ink_big=region(pg,rx0,rx1,ry0,ry1)
        off,pitch=fit_grid(ink_big)
    ink=region(pg,x0,x1,y0,y1)
    W=ink.shape[1]
    out=[]
    for k in range(32):
        a=int(round(off+k*pitch)); b=int(round(off+(k+1)*pitch))
        a=max(a,0); b=min(b,W)
        if a>=W: out.append('?'); continue
        out.append('#' if ink[:,a:b].sum()>2 else '.')
    return ''.join(out), off, pitch
