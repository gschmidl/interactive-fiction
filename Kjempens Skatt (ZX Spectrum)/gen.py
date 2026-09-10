import fitz, os
from PIL import Image
import os
# The magazine scan is not redistributed with this repository.
# Set KJEMPENS_PDF to point at it.
PDF = os.environ.get('KJEMPENS_PDF', 'Kjempens skatt (Norwegian).pdf')
doc = fitz.open(PDF)
DPI=400
regions = {
 1: [("c1",0.045,0.52,0.293,0.975), ("c2",0.487,0.985,0.293,0.975)],
 2: [("c1",0.030,0.505,0.018,0.975), ("c2",0.500,0.975,0.018,0.975)],
 3: [("c1",0.030,0.510,0.010,0.975)],
 4: [("c1",0.030,0.505,0.018,0.975), ("c2",0.500,0.975,0.018,0.610)],
 5: [("c1",0.030,0.505,0.010,0.975), ("c2",0.500,0.975,0.010,0.360)],
 6: [("c2",0.510,0.985,0.015,0.480)],
}
os.makedirs('crops',exist_ok=True)
SL=0.115
for pg,regs in regions.items():
    pm=doc[pg-1].get_pixmap(dpi=DPI)
    im=Image.frombytes("RGB",(pm.width,pm.height),pm.samples)
    W,H=im.size
    for name,x0,x1,y0,y1 in regs:
        n=max(1,round((y1-y0)/SL))
        step=(y1-y0)/n
        for k in range(n):
            a=y0+k*step-(0.004 if k else 0)
            b=y0+(k+1)*step+0.004
            im.crop((int(x0*W),int(a*H),int(x1*W),int(b*H))).save(f'crops/p{pg}{name}_{k:02d}.png')
        print(pg,name,n)
