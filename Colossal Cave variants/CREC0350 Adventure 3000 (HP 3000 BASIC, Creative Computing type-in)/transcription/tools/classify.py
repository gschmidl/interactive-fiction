"""Classify every cell against glyph templates built from the aligned samples.

Templates are per-class means of the labelled samples, each sample first
shifted onto its class mean.  A cell is compared at small x/y shifts by
normalised cross-correlation on the inner 56x36 of the 64x42 cell image.

Writes .work/ocr.npz: best label, score, second label, margin for every cell,
and .work/ocr_text.json: one string per row (spaces for empty cells).
"""
import os, sys, json
import numpy as np
from scipy import ndimage as ndi
import pagegeom as pg
from truth import load_cells, EMPTY

SHIFTS = [(dx, dy) for dy in range(-3, 4) for dx in range(-2, 3)]


def prep(img):
    x = img.astype(np.float32) / 255.0
    return ndi.gaussian_filter(x, sigma=(0, 0.8, 0.8))


def window(x, dx, dy):
    w = x[:, 4 + dy:60 + dy, 3 + dx:39 + dx].reshape(len(x), -1)
    w = w - w.mean(1, keepdims=True)
    return w / np.maximum(np.linalg.norm(w, axis=1, keepdims=True), 1e-6)


def align_to(S, mean):
    best = np.full(len(S), -2.0)
    chosen = np.zeros((len(S), mean.size), np.float32)
    for dx, dy in SHIFTS:
        W = window(S, dx, dy)
        sc = W @ mean
        upd = sc > best
        best[upd] = sc[upd]
        chosen[upd] = W[upd]
    return chosen


def build_templates(X, labels, maxsub=3):
    """Per class: the samples shifted onto the class mean, then split into up
    to maxsub sub-templates (heavy and light printing look different)."""
    classes = sorted(set(labels))
    labels = np.array(labels)
    T, owner = [], []
    for ci, ch in enumerate(classes):
        S = X[labels == ch]
        mean = window(S, 0, 0).mean(0)
        for _ in range(2):
            mean /= np.linalg.norm(mean)
            A = align_to(S, mean)
            mean = A.mean(0)
        k = min(maxsub, max(1, len(S) // 25))
        if k == 1:
            subs = [mean]
        else:
            rng = np.random.default_rng(0)
            C = A[rng.choice(len(A), k, replace=False)]
            for _ in range(10):
                lab = np.argmax(A @ C.T, 1)
                C = np.array([A[lab == j].mean(0) if np.any(lab == j) else C[j] for j in range(k)])
                C /= np.linalg.norm(C, axis=1, keepdims=True)
            subs = list(C)
        for s in subs:
            T.append(s / np.linalg.norm(s))
            owner.append(ci)
    return classes, np.array(T), np.array(owner)


def score_matrix(X, T, owner, nclass, chunk=4000):
    """Best correlation of every cell against every class (max over the
    class's sub-templates and the small shifts)."""
    n = len(X)
    best = np.full((n, nclass), -2.0, np.float32)
    for a in range(0, n, chunk):
        Xa = X[a:a + chunk]
        ba = np.full((len(Xa), len(T)), -2.0, np.float32)
        for dx, dy in SHIFTS:
            sc = window(Xa, dx, dy) @ T.T
            np.maximum(ba, sc, out=ba)
        for j in range(nclass):
            best[a:a + chunk, j] = ba[:, owner == j].max(1)
    return best


def classify(X, T, owner, nclass, chunk=4000):
    best = score_matrix(X, T, owner, nclass, chunk)
    n = len(X)
    order = np.argsort(-best, axis=1)
    top, sec = order[:, 0], order[:, 1]
    rows = np.arange(n)
    return top, best[rows, top], sec, best[rows, top] - best[rows, sec]


def write_scores(X_all, core, classes, T, owner, path=None):
    inked = np.nonzero(core >= 5.0)[0]
    S = score_matrix(X_all[inked], T, owner, len(classes))
    path = path or os.path.join(pg.WORK, "scores.npz")
    np.savez_compressed(path, idx=inked, classes=np.array(classes), S=S)
    return inked, S


def listing_samples():
    """Verified glyph samples from the program listing (same printer): used
    to strengthen the templates when working on the data-file dump pages."""
    work = os.path.normpath(os.path.join(pg.HERE, "..", ".work"))
    L = np.load(os.path.join(work, "cells.npz"))
    t = np.load(os.path.join(work, "templates.npz"))
    return prep(L["img"][t["idx"]]), list(t["labels"])


def main():
    img, meta, core, index = load_cells()
    t = np.load(os.path.join(pg.WORK, "templates.npz"))
    X_all = prep(img)
    Xs, labels = X_all[t["idx"]], list(t["labels"])
    if pg.DATASET == "data":
        XL, LL = listing_samples()
        Xs, labels = np.concatenate([Xs, XL]), labels + LL
    classes, T, owner = build_templates(Xs, labels)
    inked, S = write_scores(X_all, core, classes, T, owner)
    order = np.argsort(-S, axis=1)
    top, sec = order[:, 0], order[:, 1]
    rr = np.arange(len(inked))
    score, margin = S[rr, top], S[rr, top] - S[rr, sec]
    lab = np.full(len(img), " ", dtype="<U1")
    sc = np.zeros(len(img), np.float32); mg = np.zeros(len(img), np.float32); lab2 = np.full(len(img), " ", dtype="<U1")
    cls = np.array(classes)
    lab[inked] = cls[top]; sc[inked] = score; mg[inked] = margin; lab2[inked] = cls[sec]
    # faint or tiny marks that match nothing well are noise
    noise = (core < EMPTY) & (sc < 0.6)
    lab[noise] = " "
    import truth as _t
    if _t.ART_FLAGS is not None:
        lab[_t.ART_FLAGS & (sc < 0.75)] = " "
    np.savez_compressed(os.path.join(pg.WORK, "ocr.npz"), lab=lab, score=sc, margin=mg, lab2=lab2, core=core)
    text = {}
    for (p, c, r), cells in sorted(index.items()):
        s = "".join(lab[i] for i in cells).rstrip()
        text["%d %d %d" % (p, c, r)] = s
    json.dump(text, open(os.path.join(pg.WORK, "ocr_text.json"), "w"), indent=0)
    print("classes", len(classes), "inked", len(inked), "low score(<0.7)", int((sc[inked] < 0.7).sum()), "low margin(<0.05)", int((mg[inked] < 0.05).sum()))


if __name__ == "__main__":
    main()
