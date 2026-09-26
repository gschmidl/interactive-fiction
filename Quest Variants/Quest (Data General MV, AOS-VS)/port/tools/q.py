"""Disassemble QUEST / QUEST_SERVER with their own linker symbols."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from st import read_st, load_pr
from mvdis import dis
DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'data')
prog = sys.argv[1]
lo, hi = int(sys.argv[2], 16), int(sys.argv[3], 16)
M, _ = load_pr(os.path.join(DATA, prog + '.PR'))
syms = read_st(os.path.join(DATA, prog + '.ST'))
print(dis(M, lo, hi, syms))
