"""Which candidate glyph explains a damaged cell?

A worn ribbon or a bad photostat only removes printer dots; it never adds
them.  So the true character is the one whose clean shape covers every dot
that did print.  For each candidate this reports the fraction of the cell's
ink that falls outside the candidate's mean glyph (after the best small
shift), and draws the overlays: red = ink in the damaged cell, green = the
candidate's clean mean glyph, yellow = both.

usage: dotcheck.py page col row cell cand1 cand2 [...]   -> .work/dotcheck.png
"""
import os, sys
import numpy as np
from scipy import ndimage as ndi
from PIL import Image, ImageDraw, ImageFont
import pagegeom as pg
from truth import load_cells
from classify import prep, SHIFTS


def class_means(X, labels, idx):
    labels = np.array(labels)
    means = {}
    for ch in sorted(set(labels)):
        S = X[idx[labels == ch]]
        mean = S[:, 4:60, 3:39].mean(0)
        for _ in range(3):
            acc = np.zeros_like(mean)
            for s in S:
                best, bw = -1e9, None
                for dx, dy in SHIFTS:
                    w = s[4 + dy:60 + dy, 3 + dx:39 + dx]
                    v = float((w * mean).sum())
                    if v > best:
                        best, bw = v, w
                acc += bw
            mean = acc / len(S)
        means[ch] = mean
    return means


def main():
    page, ci, ri, k = [int(v) for v in sys.argv[1:5]]
    cands = sys.argv[5:]
    img, meta, core, index = load_cells()
    X = prep(img)
    t = np.load(os.path.join(pg.WORK, "templates.npz"))
    labels, idx = list(t["labels"]), t["idx"]
    want = set(cands)
    sel = np.array([l in want for l in labels])
    means = class_means(X, np.array(labels)[sel], idx[sel])
    cell = [i for i in index[(page, ci, ri)] if int(meta[i][3]) == k][0]
    obs = X[cell]
    rows = []
    for ch in cands:
        m = means[ch]
        best, bw, bs = -1e9, None, None
        for dx, dy in SHIFTS:
            w = obs[4 + dy:60 + dy, 3 + dx:39 + dx]
            v = float((w * m).sum() / (np.linalg.norm(w) * np.linalg.norm(m) + 1e-9))
            if v > best:
                best, bw, bs = v, w, (dx, dy)
        cover = ndi.maximum_filter(m, size=3) > 0.18 * m.max()
        ink = bw > 0.25
        unexplained = float((bw * ~cover)[ink].sum() / max(1e-9, bw[ink].sum()))
        missing = float(((m > 0.35 * m.max()) & ~(ndi.maximum_filter(bw, size=5) > 0.2)).sum() / max(1, (m > 0.35 * m.max()).sum()))
        rows.append((ch, best, unexplained, missing, bw, m))
        print("%-3s corr %.3f  ink outside glyph %.1f%%  glyph area with no ink %.1f%%  shift %s" % (
            ch, best, 100 * unexplained, 100 * missing, bs))
    S = 8
    tiles = []
    for ch, best, un, mi, bw, m in rows:
        rgb = np.zeros(bw.shape + (3,))
        rgb[..., 0] = np.clip(bw / max(bw.max(), 1e-6), 0, 1)
        rgb[..., 1] = np.clip(m / max(m.max(), 1e-6), 0, 1)
        tile = Image.fromarray((255 - rgb * 255).astype(np.uint8)).resize((bw.shape[1] * S, bw.shape[0] * S), Image.NEAREST)
        tiles.append((ch, un, tile))
    raw = Image.fromarray((255 - np.clip(obs, 0, 1) * 255).astype(np.uint8)).resize((42 * S, 64 * S), Image.NEAREST)
    W = raw.width + sum(t.width + 20 for _, _, t in tiles) + 20
    H = raw.height + 40
    canvas = Image.new("RGB", (W, H), (255, 255, 255))
    d = ImageDraw.Draw(canvas)
    try:
        font = ImageFont.truetype("consola.ttf", 28)
    except Exception:
        font = ImageFont.load_default()
    canvas.paste(raw, (0, 40)); d.text((4, 4), "cell", fill=(0, 0, 0), font=font)
    x = raw.width + 20
    for ch, un, tile in tiles:
        canvas.paste(tile, (x, 40 + (raw.height - tile.height) // 2))
        d.text((x + 4, 4), "%s: %.0f%% unexpl." % (ch, 100 * un), fill=(0, 0, 0), font=font)
        x += tile.width + 20
    out = os.path.join(pg.WORK, "dotcheck.png")
    canvas.save(out)
    print(out, canvas.size)


if __name__ == "__main__":
    main()
