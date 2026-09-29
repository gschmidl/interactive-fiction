"""Shared loader: the repaired EXPLOR core image."""
import importlib.util, os
H = os.path.dirname(os.path.abspath(__file__))
_mk = importlib.util.module_from_spec(
    importlib.util.spec_from_file_location("mk", os.path.join(H, "mkimage.py")))
_mk.__spec__.loader.exec_module(_mk)
g = _mk.words(os.path.join(H, '..', 'raw', 'explor-games.sav'))
u = _mk.words(os.path.join(H, '..', 'raw', 'explor-upl17.sav'))
_img, _lg, _lu = _mk.repair(g, u)
BLOCKS, START, _ = _mk.loadsav(_img)
MEM = {}
for _a, _d in BLOCKS:
    for _k, _w in enumerate(_d):
        MEM[_a + _k] = _w
def a5(x): return ''.join(chr((x >> (29 - 7*k)) & 0x7f) for k in range(5))
def txt(x): return ''.join(c if 32 <= ord(c) < 127 else '.' for c in a5(x))
def M(a): return MEM.get(a, 0)
