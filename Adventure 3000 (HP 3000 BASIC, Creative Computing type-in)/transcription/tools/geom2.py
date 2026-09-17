"""Rows and per-row character cells for each listing column.

Printer dots are ~0.2-0.7 on the ink map and paper grain stays under ~0.1, so
the ink is first cleaned with a soft threshold.  Each column gets a shear
(row slope), its text rows, and for every row the left edges of its character
cells, found by dynamic programming: boundaries fall where there is least ink,
with the spacing held near the character pitch.  That tolerates the few-tenths
of a pixel of pitch drift the photostats have across a column.

Writes work/cells_p<page>_c<col>.json
"""
import json, os, sys
import numpy as np
from scipy import ndimage as ndi
from scipy.signal import find_peaks
import pagegeom as pg
from columns import COLUMNS, ART, OVERFLOW, KEEP
from geom import page_ink, sheared_profile

T0, T1 = 0.10, 0.45
BAND = 24          # half height of the band used for column profiles
UP = 2             # sub-pixel lattice for the boundary DP
SLACK = 6.0        # a cell may be this much narrower or wider than the pitch (printout drifts)


def clean_ink(page):
    path = os.path.join(pg.WORK, "p%d_clean.npy" % page)
    if os.path.exists(path):
        return np.load(path).astype(np.float32)
    ink = page_ink(page)
    if pg.DATASET == "moving":
        # the table fades badly towards the bottom right: scale the ink by the
        # local strength of the print (98th percentile in a ~5-line window)
        small = ink[::8, ::8]
        loc = ndi.percentile_filter(small, 98, size=41)
        loc = ndi.uniform_filter(loc, 9)
        loc = np.kron(loc, np.ones((8, 8), np.float32))[: ink.shape[0], : ink.shape[1]]
        ink = ink * (0.55 / np.clip(loc, 0.18, 1.0))
    c = np.clip((ink - T0) / (T1 - T0), 0, 1)
    np.save(path, c.astype(np.float16))
    return c


def fit_slope(left):
    best = (-1, 0.0)
    for slope in np.arange(-0.02, 0.0201, 0.0005):
        v = float(np.sum(np.diff(sheared_profile(left, slope, 0.0)) ** 2))
        if v > best[0]:
            best = (v, float(slope))
    s0 = best[1]
    for slope in np.arange(s0 - 0.0005, s0 + 0.00051, 0.0001):
        v = float(np.sum(np.diff(sheared_profile(left, slope, 0.0)) ** 2))
        if v > best[0]:
            best = (v, float(slope))
    return best[1]


def band_profile(ink, yc, slope, xref, xa, xb):
    """Column profile of the row band centred at yc (at x = xref)."""
    w = xb - xa
    prof = np.zeros(w, np.float64)
    step = 40
    for sx in range(0, w, step):
        x = xa + sx
        yy = int(round(yc + slope * (x + step / 2 - xref)))
        prof[sx:sx + step] = ink[yy - BAND:yy + BAND, x:x + step].sum(0)[: min(step, w - sx)]
    return prof


def fold_pitch(prof, lo=35.0, hi=37.0):
    xs = np.arange(len(prof), dtype=np.float64)
    best = (-1, 0.0, 0.0)
    for pitch in np.arange(lo, hi, 0.005):
        ph = xs / pitch * 2 * np.pi
        c, s = np.sum(prof * np.cos(ph)), np.sum(prof * np.sin(ph))
        amp = np.hypot(c, s)
        if amp > best[0]:
            best = (amp, pitch, np.arctan2(s, c))
    amp, pitch, ang = best
    centre = (ang % (2 * np.pi)) / (2 * np.pi) * pitch
    return float(pitch), float((centre + pitch / 2) % pitch)


def cell_dp(prof, pitch, e0, lam, mu):
    """Boundaries b_0 < b_1 < ... over prof (x = index / UP).  Returns list of x."""
    n = len(prof)
    up = np.interp(np.arange(n * UP) / UP, np.arange(n), prof)
    N = len(up)
    INF = 1e18
    cost = np.full(N, INF)
    back = np.full(N, -1, np.int64)
    # start: first boundary within half a pitch of the column's cell-0 edge
    lo = max(0, int((e0 - pitch / 2) * UP)); hi = min(N, int((e0 + pitch / 2) * UP))
    for i in range(lo, hi):
        cost[i] = up[i] + mu * ((i / UP) - e0) ** 2
    dmin, dmax = int((pitch - SLACK) * UP), int((pitch + SLACK) * UP) + 1
    dpen = np.array([lam * (d / UP - pitch) ** 2 for d in range(dmin, dmax + 1)])
    for i in range(lo + dmin, N):
        a, b = i - dmax, i - dmin
        if b < 0:
            continue
        a = max(a, 0)
        prev = cost[a:b + 1] + dpen[: b - a + 1][::-1]
        j = int(np.argmin(prev))
        if prev[j] < INF / 2:
            cost[i] = prev[j] + up[i]
            back[i] = a + j
    # end anywhere in the last pitch of the profile
    tail = cost[max(0, N - int(pitch * UP)):]
    i = int(np.argmin(tail)) + max(0, N - int(pitch * UP))
    path = []
    while i >= 0:
        path.append(i / UP)
        i = back[i]
    return path[::-1]


MAXCOLS = 80        # widest listing row seen: the header REMs on p126 run past column 72


def masked(ink, page):
    m = ink.copy()
    for (ax0, ay0, ax1, ay1) in ART.get(page, []):
        m[ay0:ay1, ax0:ax1] = 0
    return m


def process_page(page, verbose=True):
    ink = clean_ink(page)
    mink = masked(ink, page)
    cols = []
    for ci, (x0, x1, y0, y1) in enumerate(COLUMNS[page]):
        sub = mink[y0:y1, x0:x1]
        left = sub[:, : min(sub.shape[1], 1500)]
        slope = fit_slope(left)
        prof = ndi.gaussian_filter1d(sheared_profile(left, slope, 0.0), 4)
        peaks, _ = find_peaks(prof, distance=42, prominence=max(4.0, prof.max() * 0.012), height=max(4.0, prof.max() * 0.012))
        peaks = list(peaks)
        # A short line next to a long one can fail to make its own peak.  In a
        # gap of about n+1 row spacings, look for n rows where they should be:
        # the ink centroid within 20 px of each evenly spaced position.
        fine = ndi.gaussian_filter1d(sheared_profile(left, slope, 0.0), 2)
        med = np.median(np.diff(peaks)) if len(peaks) > 2 else 62
        extra = []
        for a_, b_ in zip(peaks[:-1], peaks[1:]):
            gap = b_ - a_
            n = int(round(gap / med)) - 1
            if gap < 1.45 * med or n < 1:
                continue
            for k in range(1, n + 1):
                yexp = a_ + k * gap / (n + 1)
                lo, hi = int(yexp - 20), int(yexp + 21)
                seg = fine[lo:hi]
                base = min(fine[lo - 12:lo + 1].min() if lo >= 12 else 0, fine[hi - 1:hi + 12].min() if hi + 12 <= len(fine) else 0)
                w = np.clip(seg - base, 0, None)
                if w.sum() < 60:
                    continue
                extra.append(int(round(lo + np.sum(np.arange(len(w)) * w) / w.sum())))
        peaks = sorted(set(list(peaks) + extra))
        rows = [float(y0 + p) for p in peaks]
        field = np.zeros(440)
        for yc in rows:
            field += band_profile(mink, yc, slope, x0, x0, x0 + 440)
        pitch_f, edge_f = fold_pitch(field)
        full = np.zeros(x1 - x0)
        for yc in rows:
            full += band_profile(mink, yc, slope, x0, x0, x1)
        pitch, edge = fold_pitch(full)
        # Line numbers are right-aligned in cells 0-3 and followed by one blank
        # cell, so the end of the number marks the grid.  Some photostat strips
        # are sheared (the margin drifts sideways down the column while the rows
        # stay level), so cell 0 is fitted as a straight line in y.
        ends = []
        for yc in rows:
            xa = x0 if pg.DATASET in ("data", "moving") else x0 - 150
            p = ndi.gaussian_filter1d(band_profile(mink, yc, slope, x0, xa, x0 + 400), 1.0)
            inked = p > 3.0
            if not inked.any():
                continue
            first = int(np.argmax(inked))
            if pg.DATASET in ("data", "moving"):
                # no line numbers: the text's left edge is cell 0.  Record it
                # as if it were the end of a 4-digit number so the fit below
                # is shared (e0 = end + 3 - 4 pitches).
                ends.append((yc, xa + first - 3 + 4 * pitch_f))
                continue
            run = 0
            for x in range(first, len(p)):
                if inked[x]:
                    run = 0
                else:
                    run += 1
                    if run >= 0.95 * pitch_f:
                        ends.append((yc, xa + x - run + 1))
                        break
        ends = np.array(ends, np.float64)
        med = np.median(ends[:, 1])
        # start from the densest cluster, then refit a line through the inliers
        hist, edges_ = np.histogram(ends[:, 1], bins=np.arange(ends[:, 1].min(), ends[:, 1].max() + 14, 12))
        centre = edges_[int(np.argmax(hist))] + 6
        if pg.DATASET in ("data", "moving"):
            # indented records exist, but most rows start in cell 0
            centre = float(np.percentile(ends[:, 1], 15)) + 6
        model = (centre, 0.0)
        for _ in range(4):
            pred = model[0] + model[1] * (ends[:, 0] - y0)
            inl = np.abs(ends[:, 1] - pred) < 0.45 * pitch_f
            if inl.sum() < 3:
                break
            bb, aa = np.polyfit(ends[inl, 0] - y0, ends[inl, 1], 1)
            model = (aa, bb)
        e0_a = model[0] + 3 - 4 * pitch_f
        e0_b = model[1]
        numbered = set(float(yc) for (yc, e) in ends if abs(e - (model[0] + model[1] * (yc - y0))) < 0.45 * pitch_f)
        cols.append(dict(ci=ci, box=(x0, x1, y0, y1), slope=slope, rows=rows, pitch=pitch,
                         pitch_f=pitch_f, e0a=e0_a, e0b=e0_b, full=full,
                         inliers=int(inl.sum()), nends=len(ends), numbered=numbered))

    # A long line can run past the gutter into the next column's space, and then
    # shows up as a row of that column too.  It is overflow when the previous
    # column's text at that height runs on without a two-cell gap right up to
    # this column's cell 0, and the row does not carry an aligned line number.
    for k in range(1, len(cols)):
        c, prev = cols[k], cols[k - 1]
        keep = []
        for yc in c["rows"]:
            e0 = c["e0a"] + c["e0b"] * (yc - c["box"][2])
            P = prev["pitch"]
            xa, xb = int(e0 - 6 * P), int(e0 - 0.25 * P)
            p = ndi.gaussian_filter1d(band_profile(mink, yc, prev["slope"], prev["box"][0], xa, xb), 1.0)
            win = int(1.5 * P)
            continuous = all(p[i:i + win].max() > 2.0 for i in range(0, len(p) - win + 1, int(P / 2)))
            forced = any(abs(yc - fy) <= 15 for fy in OVERFLOW.get(page, {}).get(k, []))
            kept = any(abs(yc - fy) <= 15 for fy in KEEP.get(page, {}).get(k, []))
            if forced or (continuous and float(yc) not in c["numbered"] and not kept):
                if verbose:
                    print("   p%d c%d: row at y=%.0f is overflow from column %d" % (page, k, yc, k - 1))
                continue
            keep.append(yc)
        c["rows"] = keep
    results = []
    for c in cols:
        x0, x1, y0, y1 = c["box"]
        pitch, slope = c["pitch"], c["slope"]
        glyph = np.percentile(c["full"][c["full"] > 0], 90) / max(1, len(c["rows"]))
        lam, mu = 0.05 * glyph, 0.02 * glyph
        nxt = cols[c["ci"] + 1] if c["ci"] + 1 < len(cols) else None
        out_rows = []
        for yc in c["rows"]:
            e0 = c["e0a"] + c["e0b"] * (yc - y0)
            limit = e0 + (MAXCOLS + 0.6) * pitch
            if nxt is not None:
                # where would this row sit in the next column, and is there text there?
                ne0 = nxt["e0a"] + nxt["e0b"] * (yc - nxt["box"][2])
                ynext = yc + slope * (ne0 - x0)
                if any(abs(ynext - r2) < 34 for r2 in nxt["rows"]):
                    limit = min(limit, ne0 - 6)
            xb = int(min(limit, ink.shape[1] - 1))
            xs0 = int(min(x0, e0 - pitch))       # the grid may start left of the box
            p = band_profile(ink, yc, slope, x0, xs0, xb)
            p = ndi.gaussian_filter1d(p, 1.0)
            inked = np.nonzero(p > 0.5)[0]
            if len(inked) == 0:
                continue
            last = min(len(p) - 1, int(inked[-1]) + int(pitch))
            bounds = cell_dp(p[: last + 1], pitch, e0 - xs0, lam, mu)
            bounds = bounds[: MAXCOLS + 1]
            out_rows.append({"y": yc, "cells": [xs0 + b for b in bounds]})
        res = {"page": page, "col": c["ci"], "box": list(c["box"]), "slope": slope, "xref": x0,
               "pitch": pitch, "pitch_field": c["pitch_f"], "e0a": c["e0a"], "e0b": c["e0b"], "rows": out_rows}
        json.dump(res, open(os.path.join(pg.WORK, "cells_p%d_c%d.json" % (page, c["ci"])), "w"))
        results.append(res)
        if verbose:
            gaps = np.diff([r["y"] for r in out_rows] or [0, 0])
            sp = np.concatenate([np.diff(r["cells"]) for r in out_rows] or [np.zeros(1)])
            big = [(k + 1, int(gp)) for k, gp in enumerate(gaps) if gp > 76]
            print("p%d c%d slope %.4f pitch %.3f e0 %.1f%+.4f*y (%d/%d rows fit) rows %d maxcells %d spacing %.2f..%.2f; gaps>76 at rows %s" % (
                page, c["ci"], slope, pitch, c["e0a"], c["e0b"], c["inliers"], c["nends"], len(out_rows), max([len(r["cells"]) for r in out_rows] or [1]) - 1,
                sp.min(), sp.max(), big))
    return results


if __name__ == "__main__":
    pages = [int(a) for a in sys.argv[1:]] or sorted(COLUMNS)
    for page in pages:
        process_page(page)
