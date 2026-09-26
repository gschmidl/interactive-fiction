import mask, numpy as np
def fit(pg, x0, x1, y0, y1):
    ink = mask.region(pg,x0,x1,y0,y1)
    P = ink.sum(axis=0).astype(float)
    W = len(P)
    best=None
    for pitch in np.arange(W/33.0, W/31.0, 0.01):
        for off in np.arange(-pitch/2, pitch/2, 0.25):
            b=off+pitch*np.arange(33)
            b=b[(b>=0)&(b<W)]
            if len(b)<30: continue
            s=P[b.astype(int)].sum()/len(b)
            if best is None or s<best[0]: best=(s,off,pitch)
    return best[1],best[2]
def rowmask(pg,x0,y0,y1,off,pitch,thresh=170,minink=3):
    a=mask.page_arr(pg); H,W=a.shape
    xs=int(x0*W)
    sub=a[int(y0*H):int(y1*H), :]
    ink=(sub<thresh).astype(np.int32)
    out=[]
    for k in range(32):
        A=xs+int(round(off+k*pitch)); B=xs+int(round(off+(k+1)*pitch))
        out.append('#' if ink[:,A:B].sum()>minink else '.')
    return ''.join(out)
