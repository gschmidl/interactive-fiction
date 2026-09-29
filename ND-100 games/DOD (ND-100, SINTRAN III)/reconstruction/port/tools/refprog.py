"""The reference programs: a :PROG with RANDOM not reading the uptime.

RANDOM (ND BASIC's run-time library) seeds from MON 11, the uptime, and the
COPY SD DA after it; both become SAA, the second loading SEED: the seed is
the same every game, on the reference machine as on the port, which runs
the same image (tests\\run.py).  -REF has seed 0, -R01 to -R12 seeds 1 to 12.
"""
import struct

SEEDS = 12


def ref_of(prog, seed=0):
    w = list(struct.unpack('>%dH' % (len(prog) // 2), prog))
    seq = [0o153011, 0o146115, 0o171756, 0o006600]
    hits = [i for i in range(0x100, len(w) - 4) if w[i:i + 4] == seq]
    assert len(hits) == 1, hits
    w[hits[0]] = 0o170400                     # SAA 0
    w[hits[0] + 1] = 0o170400 | seed          # SAA seed
    return struct.pack('>%dH' % len(w), *w)


def name(program, seed):
    """DODF -> DODF-REF, DODF-R01 ... (a name in full wins over the longer ones)"""
    return '%s-REF' % program if seed == 0 else '%s-R%02d' % (program, seed)
