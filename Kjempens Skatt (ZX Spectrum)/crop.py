import fitz, sys
from PIL import Image
import os
# The magazine scan is not redistributed with this repository.
# Set KJEMPENS_PDF to point at it.
PDF = os.environ.get('KJEMPENS_PDF', 'Kjempens skatt (Norwegian).pdf')
doc = fitz.open(PDF)
def render(page, dpi):
    pm = doc[page-1].get_pixmap(dpi=dpi)
    return Image.frombytes("RGB",(pm.width,pm.height),pm.samples)
if __name__=="__main__":
    page=int(sys.argv[1]); dpi=int(sys.argv[2])
    x0,y0,x1,y1=[float(v) for v in sys.argv[3:7]]
    out=sys.argv[7]
    im=render(page,dpi)
    w,h=im.size
    im.crop((int(x0*w),int(y0*h),int(x1*w),int(y1*h))).save(out)
    print(out, im.size)
