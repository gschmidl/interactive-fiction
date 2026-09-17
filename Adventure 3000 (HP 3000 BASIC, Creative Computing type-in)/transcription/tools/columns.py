"""Column boxes of the program listing, in deskewed reading-orientation pixels
of the 600-dpi scans (see gridthumb.py).  (x0, x1, y0, y1): x0 is just left of
the line-number field, x1 is the furthest right a line of this column may run
(the listing wraps at a fixed width, so this only has to clear art and the next
column), y0..y1 bound the column vertically.  Art boxes are ignored when
looking for text rows.
"""

COLUMNS = {
    126: [(620, 3560, 650, 4480), (3560, 6500, 650, 4480)],
    128: [(620, 2620, 650, 4550), (2620, 5300, 650, 4550), (4380, 6500, 650, 3800)],
    130: [(620, 2330, 650, 4550), (2330, 3860, 650, 4550), (3860, 6500, 650, 4550)],
    132: [(620, 2610, 650, 4550), (2610, 4300, 650, 4550), (4300, 6500, 650, 4550)],
    134: [(620, 2400, 650, 4550), (2400, 4300, 650, 4550), (4300, 6500, 650, 4550)],
    136: [(620, 2960, 650, 4550), (2960, 4530, 650, 4550), (4530, 6500, 650, 4550)],
    138: [(620, 3500, 650, 4550)],
}

ART = {
    126: [(2950, 2400, 3585, 3500), (5150, 2990, 6060, 3960)],
    128: [(1760, 700, 2600, 1800), (5420, 2900, 6520, 4350)],
    130: [(4880, 1250, 6350, 2750), (1400, 3550, 2420, 4470)],
    132: [(1650, 2200, 2640, 3630), (5400, 1950, 6330, 3330)],
    134: [(1760, 1720, 2350, 2340)],
    136: [(1795, 3050, 2995, 4455)],
    138: [(2080, 2380, 2820, 3295)],
}

# the running head "Adventure, con't..." printed sideways at the right edge,
# measured per page (bold ink bounding box + 15 px); a generic box overlapped
# the ends of long third-column rows
RUNNING_HEAD = {
    126: (6225, 680, 6335, 1305),
    128: (6235, 750, 6345, 1370),
    130: (6160, 670, 6270, 1290),
    132: (6190, 695, 6300, 1315),
    134: (6230, 695, 6340, 1315),
    136: (6330, 700, 6440, 1320),
    138: (6195, 690, 6310, 1310),
}
for _p in ART:
    ART[_p].append(RUNNING_HEAD[_p])

# Rows that are the tail of a long line from the column to the left even though
# the automatic test lets them through (a word break in the overflowing text
# happened to line up with this column's line-number field).  page: {col: [y]}
OVERFLOW = {
    134: {1: [1731]},
}

# Genuine rows the overflow test would drop: a long line in the column to the
# left ends right against them.  All 19 automatically flagged rows were checked
# by eye on 2026-09-17 (work/overflow_review.png); only this one was wrong.
KEEP = {
    134: {1: [2833]},     # 7020 REM DRAGON & RUG, right after 6490's closing quote
}


# ---- the data-file dumps (ADV_SET=data) --------------------------------
# Printed by the same listing device, but no line numbers: cell 0 is found
# from the left edge of the text instead.  The movement table on p124 is a
# separate layout and is not in this set.
DATA_COLUMNS = {
    112: [(600, 3550, 650, 4550), (3560, 6600, 650, 4550)],
    114: [(600, 3550, 650, 4550), (3560, 6600, 650, 4550)],
    116: [(600, 3550, 650, 4550), (3560, 6600, 650, 4550)],
    118: [(600, 3550, 650, 4550), (3560, 6600, 650, 4550)],
    120: [(600, 3550, 650, 4550), (3560, 6600, 650, 4550)],
    122: [(600, 3550, 650, 4550), (3560, 6600, 650, 2960)],
}
DATA_ART = {
    # measured drawing extents (dense-ink blobs at 1/8 scale) + 20 px
    112: [(2690, 640, 3615, 2150), (2410, 3220, 3655, 4440), (5520, 2790, 6100, 3550)],
    114: [(2595, 1110, 3525, 2660), (2685, 3245, 3395, 4150)],
    116: [(2555, 3050, 3600, 4430), (5555, 1890, 6270, 2780)],
    118: [(2925, 2590, 3605, 3295)],
    120: [(6060, 1450, 6320, 1610), (6140, 2290, 6280, 2460)],     # handwritten margin notes
    122: [(3555, 3020, 6240, 4440)],
}
DATA_RUNNING_HEAD = {
    112: (6240, 670, 6355, 1290),
    114: (6190, 645, 6300, 1270),
    116: (6205, 650, 6315, 1280),
    118: (6175, 715, 6285, 1340),
    120: (6235, 690, 6345, 1315),
    122: (6185, 700, 6290, 1325),
}
for _p in DATA_ART:
    DATA_ART[_p].append(DATA_RUNNING_HEAD[_p])

# ---- the movement table (ADV_SET=moving): output of the author's LIST program
MOVING_COLUMNS = {
    124: [(600, 3480, 1950, 4600), (3480, 6120, 780, 4600)],
}
MOVING_ART = {
    124: [(6180, 690, 6360, 1320)],       # running head
}

import os as _os
if _os.environ.get("ADV_SET", "listing") == "data":
    COLUMNS, ART = DATA_COLUMNS, DATA_ART
    OVERFLOW, KEEP = {}, {}
elif _os.environ.get("ADV_SET", "listing") == "moving":
    COLUMNS, ART = MOVING_COLUMNS, MOVING_ART
    OVERFLOW, KEEP = {}, {}
