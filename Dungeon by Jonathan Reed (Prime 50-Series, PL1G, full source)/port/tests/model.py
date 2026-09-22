"""Model Reed's INIT under different PL/I arithmetic hypotheses and compare
with what the original printed on PRIMOS (position + ESP map)."""
import struct, sys

def f32(x):
    return struct.unpack("f", struct.pack("f", x))[0]

class Arith:
    """seed stays an exact integer; only rnd and FLOOR(rnd*K) differ"""
    def __init__(self, mode):
        self.mode = mode
    def floor_mul(self, seed, k):
        r = seed % 100
        if self.mode == "dec":            # exact decimal: rnd = r/100
            return (r * k) // 100
        if self.mode == "f32":
            rnd = f32(r * f32(0.01))
            return int(f32(rnd * k) // 1)
        rnd = r * 0.01                    # double
        return int(rnd * k // 1)

class Game:
    def __init__(self, seed, arith):
        self.seed = seed
        self.a = arith
        self.m = [[[0] * 11 for _ in range(11)] for _ in range(11)]
        self.t = [[[0] * 11 for _ in range(11)] for _ in range(11)]
        self.draws = 0
    def rand(self):
        s = self.seed
        self.seed = (s + s // 100 + 12727) % 100000
        self.draws += 1
    def fl(self, k):                      # FLOOR(rnd*k) of the current rnd
        return self.a.floor_mul(self.seed, k)
    def thcord(self):
        self.rand(); x = self.fl(10) + 1
        self.rand()
        for _ in range(self.fl(20)):
            self.rand()
        y = self.fl(10) + 1
        self.rand()
        for _ in range(self.fl(20)):
            self.rand()
        z = self.fl(10) + 1
        return x, y, z
    def init(self, short=True):
        for _ in range(75):
            x, y, z = self.thcord(); self.m[x][y][z] = 1
            for _ in range(self.fl(20)): self.rand()
            x, y, z = self.thcord(); self.m[x][y][z] = 2
            for _ in range(self.fl(20)): self.rand()
            x, y, z = self.thcord(); self.t[x][y][z] = 1
            for _ in range(self.fl(20)): self.rand()
        for _ in range(50):
            x, y, z = self.thcord()
            for _ in range(self.fl(20)): self.rand()
            self.m[x][y][z] = 2
            for _ in range(self.fl(20) + 1): self.rand()
            x, y, z = self.thcord(); self.t[x][y][z] = 1
        for _ in range(self.fl(20) + 1): self.rand()
        for _ in range(12):
            x, y, z = self.thcord(); self.m[x][y][z] = 4
            x, y, z = self.thcord(); self.m[x][y][z] = 1
            x, y, z = self.thcord(); self.m[x][y][z] = 3
            for _ in range(self.fl(20) + 1): self.rand()
        for _ in range(self.fl(20) + 1): self.rand()
        for _ in range(12):
            x, y, z = self.thcord(); self.m[x][y][z] = 7
            x, y, z = self.thcord(); self.m[x][y][z] = 3
            for _ in range(self.fl(20) + 1): self.rand()
        v = 5 if short else 3
        for _ in range(v):
            for _ in range(self.fl(20) + 1): self.rand()
            x, y, z = self.thcord(); self.m[x][y][z] = 5
            for _ in range(self.fl(20) + 1): self.rand()
            x, y, z = self.thcord(); self.m[x][y][z] = 6
        return self.thcord()
    def esp(self, pos):
        out = []
        for x in range(1, 11):
            row = ""
            for y in range(1, 11):
                if (x, y) == (pos[0], pos[1]):
                    row += "@"
                elif self.m[x][y][pos[2]] > 0:
                    row += "*"
                else:
                    row += "."
            out.append(row)
        return out

TARGET_POS = (8, 4, 5)
TARGET_MAP = ["..........", "**.......*", ".*..*.*...", "...*......", "..*.*.....",
              "........*.", "**..*.....", "..*@..*...", "......*...", "....*....."]

for mode in ("dec", "f32", "f64"):
    seed0 = 1 * 1397 + 192
    g = Game(seed0, Arith(mode))
    pos = g.init(short=True)
    m = g.esp(pos)
    print("%-4s pos %-12s draws %-6d map %s" % (mode, pos, g.draws, "MATCH" if m == TARGET_MAP else "no"))
    if pos == TARGET_POS:
        print("     position matches!")
        for r in m: print("     " + r)
