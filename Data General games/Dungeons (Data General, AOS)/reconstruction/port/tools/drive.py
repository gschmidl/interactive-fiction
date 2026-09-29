"""Play Dungeons through a pipe and record the keys actually used.

Answers "Do you dare to enter?" and "play again?" with Y (until the game
count is reached, then N) and otherwise picks random commands.  With -Z the
emulator's clock is frozen, so replaying the recorded keys reproduces the
session exactly -- tests/long.keys was made this way:

    python tools/drive.py 3 4 424242 > tests/long.keys
"""
import subprocess, sys, threading, time, random, queue

level, games, seed = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
rnd = random.Random(seed)
p = subprocess.Popen(['./aosvs16.exe', '-Z', '-d', 'data', '-s', '.', 'data/DG.PR', level],
                     stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
q = queue.Queue()
def reader():
    while True:
        b = p.stdout.read(1)
        if not b: q.put(None); return
        q.put(b)
threading.Thread(target=reader, daemon=True).start()

text, keys, played, turns = '', [], 0, 0
while True:
    try:
        b = q.get(timeout=0.15)
    except queue.Empty:
        tail = text[-200:]
        if 'Do you dare to enter?' in tail:
            k = 'Y'; played += 1
        elif 'play again?' in tail:
            k = 'Y' if played < games else 'N'
        else:
            k = rnd.choice('AMLCRTGBPH' + 'MMMMMMLTTT')
            turns += 1
            if turns > 3000: k = 'Q'
        if k == 'M': k += rnd.choice('NSEW')
        elif k == 'G': k += rnd.choice('123456')
        keys.append(k)
        text = ''
        try:
            p.stdin.write(k.encode()); p.stdin.flush()
        except OSError:
            break
        continue
    if b is None: break
    text += b.decode('latin-1')
sys.stdout.write(''.join(keys))
