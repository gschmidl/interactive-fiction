"""Play ADVENTURE/3000 on the real HP 3000 and save the transcript.

usage: python play.py <moves.txt> [transcript.txt]

Each line of the moves file is one answer typed at the game's next prompt: the
main loop's ">" (which the program brackets with terminal-enhancement escapes)
or INPUT's "?".  Waiting for the prompt matters because MPE's terminal driver
drops anything typed before the program asks for it.
"""
import os, re, sys, time

# mpe.py is the little expect-style telnet driver for the simulator's ADCC
# ports; point HP3000_TOOLS at the directory holding it (see README.md).
sys.path.insert(0, os.environ.get("HP3000_TOOLS", os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "_HP3000_work", "tools")))
from mpe import MPE

PROMPT = re.compile(r"&dB>|\r\n\? |\r\n\?$|Yes or No-")


def main(moves_path, out_path=None, prog="ADV3000"):
    moves = [l.rstrip("\n") for l in open(moves_path, encoding="ascii") if l.strip() != ""]
    out_path = out_path or os.path.splitext(moves_path)[0] + ".transcript.txt"
    m = MPE()
    buf = []

    def grab(t=1.0):
        s = m.read(t)
        buf.append(s)
        return s

    m.read(2)
    for attempt in range(6):          # MPE needs a moment after a logoff
        m.sendline("")
        try:
            m.expect(":", 10)
            break
        except TimeoutError:
            time.sleep(2)
    else:
        raise TimeoutError("no MPE prompt")
    m.sendline("HELLO MANAGER.SYS")
    m.expect("Reserved\\.\r\n:", 40)
    m.sendline("RUN BASIC")
    m.expect("\r\n>", 60)
    m.sendline("GET " + prog)
    m.expect("\r\n>", 120)
    buf = []
    m.sendline("RUN")

    for mv in moves:
        # wait for a prompt, then answer it
        end = time.time() + 120
        while time.time() < end:
            grab(0.5)
            if PROMPT.search("".join(buf)[-400:]):
                break
        m.sendline(mv)
    grab(4)
    m.sendline("")
    grab(2)
    m.sendline("EXIT")
    grab(2)
    m.sendline("BYE")
    grab(2)
    m.close()

    text = "".join(buf)
    text = text.replace("\x1b&dB", "").replace("\x1b&d@", "")
    open(out_path, "w", encoding="utf-8", newline="").write(text)
    print(text)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else None, sys.argv[3] if len(sys.argv) > 3 else "ADV3000")
