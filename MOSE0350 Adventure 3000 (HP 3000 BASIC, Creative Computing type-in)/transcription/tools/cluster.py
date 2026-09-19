"""k-means over the inked cells, and contact sheets of the clusters for labelling.

usage: cluster.py [k]      writes .work/clusters.npz and .work/clusters_<n>.png
"""
import os, sys
import numpy as np
from scipy import ndimage as ndi
from PIL import Image, ImageDraw
import pagegeom as pg

CORE_MIN = 3.0


def features(img):
    x = img.astype(np.float32) / 255.0
    x = ndi.gaussian_filter(x, sigma=(0, 1.0, 1.0))
    x = ndi.zoom(x, (1, 2 / 3, 2 / 3), order=1)   # 43 x 28
    f = x.reshape(len(x), -1)
    f = f - f.mean(1, keepdims=True)
    n = np.linalg.norm(f, axis=1, keepdims=True)
    return f / np.maximum(n, 1e-6)


def kmeans(f, k, iters=25, seed=1):
    rng = np.random.default_rng(seed)
    # k-means++ on a sample
    samp = f[rng.choice(len(f), min(len(f), 6000), replace=False)]
    cent = [samp[rng.integers(len(samp))]]
    d2 = 2 - 2 * samp @ cent[0]
    for _ in range(1, k):
        p = np.maximum(d2, 0) ** 2
        cent.append(samp[rng.choice(len(samp), p=p / p.sum())])
        d2 = np.minimum(d2, 2 - 2 * samp @ cent[-1])
    C = np.array(cent)
    for it in range(iters):
        sim = f @ C.T
        lab = np.argmax(sim, 1)
        newC = np.zeros_like(C)
        np.add.at(newC, lab, f)
        cnt = np.bincount(lab, minlength=k)
        for j in range(k):
            if cnt[j] == 0:
                # re-seed an empty cluster with the worst-fitting point
                worst = np.argmin(sim[np.arange(len(f)), lab])
                newC[j] = f[worst]
                cnt[j] = 1
        C = newC / np.linalg.norm(newC, axis=1, keepdims=True)
    sim = f @ C.T
    lab = np.argmax(sim, 1)
    return C, lab, sim[np.arange(len(f)), lab]


def sheets(img, idx, lab, fit, k, per=12, rows_per_sheet=40, scale=0.6):
    order = sorted(range(k), key=lambda j: -np.sum(lab == j))
    gw, gh = int(42 * scale), int(64 * scale)
    lw = 70
    colw = lw + per * (gw + 2) + 10
    n_sheets = (len(order) + 2 * rows_per_sheet - 1) // (2 * rows_per_sheet)
    for s in range(n_sheets):
        sheet = Image.new("L", (2 * colw, rows_per_sheet * (gh + 4) + 4), 255)
        d = ImageDraw.Draw(sheet)
        for slot, j in enumerate(order[s * 2 * rows_per_sheet:(s + 1) * 2 * rows_per_sheet]):
            cx = (slot // rows_per_sheet) * colw
            cy = (slot % rows_per_sheet) * (gh + 4) + 2
            mem = np.nonzero(lab == j)[0]
            d.text((cx + 2, cy + gh // 2 - 6), "%d:%d" % (j, len(mem)), fill=0)
            if len(mem) == 0:
                continue
            # spread picks across the fit range: best, typical, worst
            mem = mem[np.argsort(-fit[mem])]
            picks = mem[np.linspace(0, len(mem) - 1, min(per, len(mem))).astype(int)]
            for t, m in enumerate(picks):
                g = Image.fromarray(255 - img[idx[m]]).resize((gw, gh), Image.LANCZOS)
                sheet.paste(g, (cx + lw + t * (gw + 2), cy))
        sheet.save(os.path.join(pg.WORK, "clusters_%d.png" % s))
    return order


def main():
    k = int(sys.argv[1]) if len(sys.argv) > 1 else 160
    d = np.load(os.path.join(pg.WORK, "cells.npz"))
    img, meta = d["img"], d["meta"]
    core = img[:, 10:54, 3:39].reshape(len(img), -1).sum(1) / 255.0
    idx = np.nonzero((core >= CORE_MIN) & (core < 700))[0]
    f = features(img[idx])
    C, lab, fit = kmeans(f, k)
    order = sheets(img, idx, lab, fit, k)
    np.savez_compressed(os.path.join(pg.WORK, "clusters.npz"), idx=idx, lab=lab, fit=fit, C=C, order=np.array(order))
    print("inked cells", len(idx), "clusters", k, "sizes", sorted(np.bincount(lab, minlength=k))[::-1][:20])


if __name__ == "__main__":
    main()
