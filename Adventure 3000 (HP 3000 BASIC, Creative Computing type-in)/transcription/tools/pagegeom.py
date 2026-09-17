"""Page geometry for the ADVENTURE/3000 listing scans.

The listing pages of Creative Computing (Nov 1979) are printed sideways.  In the
archive.org original scan (600 dpi, 5096x6600 portrait) the text reads after a
90-degree clockwise turn.  Everything here works in that reading orientation.
"""
import os
import numpy as np
from PIL import Image
from scipy import ndimage as ndi

HERE = os.path.dirname(os.path.abspath(__file__))
PROJ = os.path.normpath(os.path.join(HERE, "..", ".."))
SCANS = os.path.join(PROJ, "scans")
# ADV_SET=data switches every tool to the data-file dump pages (pp. 112-124),
# with their own work directory, column boxes and transcription file.
DATASET = os.environ.get("ADV_SET", "listing")
WORK = os.path.join(PROJ, "transcription", {"listing": "work", "data": "work_data", "moving": "work_moving"}[DATASET])
os.makedirs(WORK, exist_ok=True)

Image.MAX_IMAGE_PIXELS = None


def scan_path(page, source="tif"):
    if source == "tif":
        p = os.path.join(SCANS, "cc79_600dpi_tif", "p%d.tif" % page)
        if os.path.exists(p):
            return p
        source = "jp2"
    if source == "jp2":
        return os.path.join(SCANS, "cc79_600dpi_jp2", "p%d.jp2" % page)
    if source == "mf":
        return os.path.join(SCANS, "cc79_microfilm800_jp2", "p%d.jp2" % page)
    if source == "better":
        return os.path.join(SCANS, "cc79_better_jp2", "p%d.jp2" % page)
    raise ValueError(source)


def load_gray(page, source="tif"):
    """Grayscale page in reading orientation, float32 0..255."""
    im = Image.open(scan_path(page, source)).convert("L")
    if im.height > im.width:
        im = im.rotate(-90, expand=True)
    return np.asarray(im, np.float32)


def ink_map(a, bg_size=61):
    """0 = paper, 1 = full ink.  Paper level from a grey closing (max filter)
    so that uneven lighting and the faint regions are normalised locally."""
    small = a[::4, ::4]
    bg = ndi.grey_closing(small, size=(bg_size // 4 * 2 + 1, bg_size // 4 * 2 + 1))
    bg = ndi.uniform_filter(bg, 9)
    bg = np.kron(bg, np.ones((4, 4), np.float32))[: a.shape[0], : a.shape[1]]
    if bg.shape != a.shape:
        pad = np.zeros_like(a)
        pad[: bg.shape[0], : bg.shape[1]] = bg
        bg = pad
    ink = (bg - a) / np.maximum(bg - 40.0, 40.0)
    return np.clip(ink, 0, 1)


def deskew_angle(ink, box=None, span=1.0, step=0.02):
    """Angle (degrees, for ndimage.rotate) that makes text rows horizontal."""
    sub = ink if box is None else ink[box[1]:box[3], box[0]:box[2]]
    sub = sub[::2, ::2]
    best = (-1, 0.0)
    for ang in np.arange(-span, span + 1e-9, step):
        r = ndi.rotate(sub, ang, reshape=False, order=1)
        prof = r.sum(1)
        v = float(np.sum(np.diff(prof) ** 2))
        if v > best[0]:
            best = (v, float(ang))
    return best[1]
