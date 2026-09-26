import mask, numpy as np

def column_ink(pg, x0, x1, thresh=170):
    a = mask.page_arr(pg); H, W = a.shape
    sub = a[:, int(x0*W):int(x1*W)]
    return (sub < thresh), H, W

def find_rows(ink, y0, y1, H, minpix=2):
    seg = ink[int(y0*H):int(y1*H)]
    rs = seg.sum(axis=1); on = rs > minpix
    out=[]; i=0
    while i < len(on):
        if on[i]:
            j=i
            while j<len(on) and on[j]: j+=1
            out.append((int(y0*H)+i, int(y0*H)+j)); i=j
        else: i+=1
    return out

def fit_grid(ink, rows, Prange=(24.0,30.0)):
    P = np.zeros(ink.shape[1])
    firsts=[]
    for a,b in rows:
        col = ink[a:b].sum(axis=0)
        P += col
        nz=np.nonzero(col>0)[0]
        if len(nz): firsts.append(nz[0])
    lo = int(np.percentile(firsts,3))
    nzall=np.nonzero(P>0)[0]; hi=nzall[-1]
    best=None
    for pitch in np.arange(*Prange, 0.005):
        c0 = lo - 0.14*pitch
        end = c0+32*pitch
        if end < hi: continue
        bnds = c0 + pitch*np.arange(33)
        bi = np.clip(bnds.astype(int),0,len(P)-1)
        s = P[bi].sum() + (end-hi)*2
        if best is None or s < best[0]: best=(s,c0,pitch)
    return best[1],best[2]

def masks(pg,x0,x1,y0,y1,minink=4,thresh=170):
    ink,H,W = column_ink(pg,x0,x1,thresh)
    rows = find_rows(ink,y0,y1,H)
    c0,pitch = fit_grid(ink,rows)
    out=[]
    for a,b in rows:
        r = ink[a:b]
        m=''
        for k in range(32):
            A=int(round(c0+k*pitch)); B=int(round(c0+(k+1)*pitch))
            A=max(A,0); B=min(B,r.shape[1])
            m += '#' if (B>A and r[:,A:B].sum()>minink) else '.'
        out.append((a,b,m))
    return out,c0,pitch

def fit_rows(ink, y0, y1, H, Rrange=(24.0,34.0)):
    seg = ink[int(y0*H):int(y1*H)]
    R = seg.sum(axis=1).astype(float)
    n=len(R)
    best=None
    for pitch in np.arange(*Rrange,0.01):
        for ph in np.arange(0,pitch,0.5):
            idx=(ph+pitch*np.arange(int((n-ph)/pitch))).astype(int)
            if len(idx)<5: continue
            s=R[idx].mean()
            if best is None or s<best[0]: best=(s,ph,pitch)
    s,ph,pitch=best
    rows=[]
    k=0
    while ph+ (k+1)*pitch <= n:
        a=int(ph+k*pitch); b=int(ph+(k+1)*pitch)
        rows.append((int(y0*H)+a,int(y0*H)+b)); k+=1
    return rows,pitch

def masks2(pg,x0,x1,y0,y1,minink=4,thresh=170):
    ink,H,W = column_ink(pg,x0,x1,thresh)
    rows0 = find_rows(ink,y0,y1,H)
    c0,pitch = fit_grid(ink,rows0)
    rows,rp = fit_rows(ink,y0,y1,H)
    out=[]
    for a,b in rows:
        r=ink[a:b]
        if r.sum()<8: 
            out.append((a,b,None)); continue
        m=''
        for k in range(32):
            A=int(round(c0+k*pitch)); B=int(round(c0+(k+1)*pitch))
            A=max(A,0); B=min(B,r.shape[1])
            m += '#' if (B>A and r[:,A:B].sum()>minink) else '.'
        out.append((a,b,m))
    return out,c0,pitch,rp

def masks3(pg,x0,x1,y0,y1,minink=4,thresh=170):
    ink,H,W = column_ink(pg,x0,x1,thresh)
    bands = find_rows(ink,y0,y1,H)
    c0,pitch = fit_grid(ink,bands)
    hs=sorted(b-a for a,b in bands)
    med=hs[len(hs)//2]
    # estimate row pitch from band starts
    starts=[a for a,b in bands]
    diffs=sorted(starts[i+1]-starts[i] for i in range(len(starts)-1))
    rp=diffs[len(diffs)//2]
    rows=[]
    for a,b in bands:
        n=max(1,int(round((b-a)/rp)))
        if (b-a) < 1.45*rp: n=1
        for i in range(n):
            rows.append((a+int((b-a)*i/n), a+int((b-a)*(i+1)/n)))
    out=[]
    for a,b in rows:
        r=ink[a:b]
        m=''
        for k in range(32):
            A=int(round(c0+k*pitch)); B=int(round(c0+(k+1)*pitch))
            A=max(A,0); B=min(B,r.shape[1])
            m += '#' if (B>A and r[:,A:B].sum()>minink) else '.'
        out.append((a,b,m))
    return out,c0,pitch,rp

def fit_grid2(ink, bands):
    firsts=[]; lasts=[]
    for a,b in bands:
        col=ink[a:b].sum(axis=0)
        nz=np.nonzero(col>0)[0]
        if len(nz)<2: continue
        if (nz[-1]-nz[0]) > 0 and (col>0).sum() > 0.9*(nz[-1]-nz[0]+1): continue  # solid graphic
        firsts.append(nz[0]); lasts.append(nz[-1])
    firsts=np.array(firsts); lasts=np.array(lasts)
    lo=int(np.percentile(firsts,10)); hi=int(np.percentile(lasts,99))
    pitch=(hi-lo)/31.72
    c0=lo-0.14*pitch
    return c0,pitch,lo,hi

def masks4(pg,x0,x1,y0,y1,minink=4,thresh=170):
    ink,H,W = column_ink(pg,x0,x1,thresh)
    bands = find_rows(ink,y0,y1,H)
    c0,pitch,lo,hi = fit_grid2(ink,bands)
    starts=[a for a,b in bands]
    diffs=sorted(starts[i+1]-starts[i] for i in range(len(starts)-1))
    rp=diffs[len(diffs)//2]
    rows=[]
    for a,b in bands:
        n=max(1,int(round((b-a)/rp)))
        if (b-a) < 1.45*rp: n=1
        for i in range(n):
            rows.append((a+int((b-a)*i/n), a+int((b-a)*(i+1)/n)))
    out=[]
    for a,b in rows:
        r=ink[a:b]
        m=''
        for k in range(32):
            A=int(round(c0+k*pitch)); B=int(round(c0+(k+1)*pitch))
            A=max(A,0); B=min(B,r.shape[1])
            m += '#' if (B>A and r[:,A:B].sum()>minink) else '.'
        out.append((a,b,m))
    return out,c0,pitch,rp

def masks5(pg,x0,x1,y0,y1,minink=6,thresh=170,trim=3):
    ink,H,W = column_ink(pg,x0,x1,thresh)
    bands = find_rows(ink,y0,y1,H)
    c0,pitch,lo,hi = fit_grid2(ink,bands)
    starts=[a for a,b in bands]
    diffs=sorted(starts[i+1]-starts[i] for i in range(len(starts)-1))
    rp=diffs[len(diffs)//2]
    R=ink.sum(axis=1)
    rows=[]
    for a,b in bands:
        n=max(1,int(round((b-a)/rp)))
        if (b-a) < 1.45*rp: n=1
        if n==1: rows.append((a,b)); continue
        cuts=[a]
        for i in range(1,n):
            g=a+int((b-a)*i/n)
            w=max(2,int(rp*0.18))
            seg=R[g-w:g+w+1]
            cuts.append(g-w+int(np.argmin(seg)))
        cuts.append(b)
        for i in range(n): rows.append((cuts[i],cuts[i+1]))
    out=[]
    for a,b in rows:
        aa=a+trim; bb=b-trim
        if bb<=aa: aa,bb=a,b
        r=ink[aa:bb]
        m=''
        for k in range(32):
            A=int(round(c0+k*pitch)); B=int(round(c0+(k+1)*pitch))
            A=max(A,0); B=min(B,r.shape[1])
            m += '#' if (B>A and r[:,A:B].sum()>minink) else '.'
        out.append((a,b,m))
    return out,c0,pitch,rp

def fit_grid3(ink, bands, Prange=(24.5,29.5)):
    P=np.zeros(ink.shape[1])
    firsts=[]
    for a,b in bands:
        col=ink[a:b].sum(axis=0)
        nz=np.nonzero(col>0)[0]
        if len(nz)<2: continue
        if (col>0).sum() > 0.9*(nz[-1]-nz[0]+1): continue
        P+=col; firsts.append(nz[0])
    lo=float(np.percentile(firsts,10))
    nz=np.nonzero(P>0)[0]; hi=nz[-1]
    Pc=np.cumsum(np.concatenate([[0],P]))
    best=None
    for pitch in np.arange(*Prange,0.005):
        for c0 in np.arange(lo-0.6*pitch, lo+0.2*pitch, 0.25):
            if c0+32*pitch < hi-0.3*pitch: continue
            b=c0+pitch*np.arange(33)
            bi=np.clip(np.round(b).astype(int),1,len(P)-1)
            s=(Pc[bi+1]-Pc[bi-1]).sum()/33.0
            if best is None or s<best[0]: best=(s,c0,pitch)
    return best[1],best[2]

def masks6(pg,x0,x1,y0,y1,minink=8,thresh=170):
    ink,H,W = column_ink(pg,x0,x1,thresh)
    bands = find_rows(ink,y0,y1,H)
    c0,pitch = fit_grid3(ink,bands)
    starts=[a for a,b in bands]
    diffs=np.array([starts[i+1]-starts[i] for i in range(len(starts)-1)])
    rp=float(np.percentile(diffs[diffs>15],20))
    R=ink.sum(axis=1)
    rows=[]
    for a,b in bands:
        n=max(1,int(round((b-a)/rp)))
        if (b-a) < 1.45*rp: n=1
        if n==1: rows.append((a,b)); continue
        cuts=[a]
        for i in range(1,n):
            g=a+int((b-a)*i/n); w=max(2,int(rp*0.18))
            cuts.append(g-w+int(np.argmin(R[g-w:g+w+1])))
        cuts.append(b)
        for i in range(n): rows.append((cuts[i],cuts[i+1]))
    out=[]
    for a,b in rows:
        r=ink[a:b]; m=''
        for k in range(32):
            A=int(round(c0+k*pitch)); B=int(round(c0+(k+1)*pitch))
            A=max(A,0); B=min(B,r.shape[1])
            m += '#' if (B>A and r[:,A:B].sum()>minink) else '.'
        out.append((a,b,m))
    return out,c0,pitch,rp
