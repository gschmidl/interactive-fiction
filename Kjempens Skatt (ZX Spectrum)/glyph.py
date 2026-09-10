import grid2, json, fitz, numpy as np
from PIL import Image
import os
# The magazine scan is not redistributed with this repository.
# Set KJEMPENS_PDF to point at it.
PDF = os.environ.get('KJEMPENS_PDF', 'Kjempens skatt (Norwegian).pdf')
# Spectrum character ROMs, likewise not redistributed here.
ROM  = os.environ.get('SPECTRUM_ROM', 'spectrum48.rom')
ROMN = os.environ.get('SPECTRUM_NORDIC_ROM', 'spectrum48.nordic.rom')
DOC=fitz.open(PDF)
FONT=open(ROM,'rb').read()[0x3D00:0x3D00+96*8]
NORD=open(ROMN,'rb').read()[0x3D00:0x3D00+96*8]
PATCH={'p5c1':[29]}
def rows(region):
    info=json.load(open('regions2.json'))[region]
    o,c0,p,_=grid2.masks6(info['pg'],info['x0'],info['x1'],info['y0'],info['y1'],minink=8)
    o=[r for r in o if (r[1]-r[0])>=12]
    for i in PATCH.get(region,[]): o.insert(i,None)
    return o,c0,p,info
def rowidx(region,text):
    rs=[l for l in open(f'rows_{region}.txt').read().split('\n')]
    for i,r in enumerate(rs):
        if text in r: return i
def cellbits(region, ri, k, top_frac=1/6, height_frac=8/6, dpi=1200):
    o,c0,p,info=rows(region); a,b,m=o[ri]
    h=b-a
    top=a-h*top_frac; hgt=h*height_frac
    page=DOC[info['pg']-1]; W300=page.rect.width/72*300
    x0=(info['x0']*W300+c0+k*p)/300*72; x1=(info['x0']*W300+c0+(k+1)*p)/300*72
    pm=page.get_pixmap(dpi=dpi, clip=fitz.Rect(x0, top/300*72, x1, (top+hgt)/300*72))
    im=Image.frombytes("RGB",(pm.width,pm.height),pm.samples).convert('L').resize((8,8),Image.BOX)
    return 255-np.array(im,dtype=float)
def match(v, font=FONT, top=5):
    v=(v-v.mean())/(v.std()+1e-6)
    res=[]
    for i in range(96):
        g=np.array([[1.0 if font[i*8+r]&(0x80>>c) else 0.0 for c in range(8)] for r in range(8)])
        g=(g-g.mean())/(g.std()+1e-6)
        res.append(((v*g).sum()/64, 32+i))
    res.sort(reverse=True)
    return res[:top]
