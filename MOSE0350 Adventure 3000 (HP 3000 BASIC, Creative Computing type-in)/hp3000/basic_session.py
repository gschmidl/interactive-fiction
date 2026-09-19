"""Drive BASIC/3000 on the SIMH HP 3000 over the ADCC telnet port.

usage:  python basic_session.py <script.txt> [--keep]

Script lines are sent one at a time, each waiting for the interpreter's prompt
at the start of a line, so nothing is typed before the machine is ready to read
it (MPE's terminal driver has no type-ahead: anything typed early is lost).
Everything the machine says is printed and written to <script>.log.

Script directives:
    #MPE           following lines go to MPE, not BASIC
    #BASIC         :RUN BASIC and switch to BASIC prompts
    #SLEEP n       wait n seconds
    #FEED text     wait for the running program's "@@" prompt, then send text
    #WAITPROMPT    wait for BASIC's prompt (after a program that fed data ends)
    #SEND text     send text with no wait (INPUT data that needs no handshake)
"""
import os, sys, time

# mpe.py is the little expect-style telnet driver for the simulator's ADCC
# ports; point HP3000_TOOLS at the directory holding it (see README.md).
sys.path.insert(0, os.environ.get("HP3000_TOOLS", os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "_HP3000_work", "tools")))
from mpe import MPE

BPROMPT = "\r\n>"          # BASIC's prompt, only ever at the start of a line
MPROMPT = "\r\n:"          # MPE's
FEEDPROMPT = "@@"          # what our loader programs print before each LINPUT


def main(path, keep=False):
    lines = [l.rstrip("\n").rstrip("\r") for l in open(path, encoding="ascii").read().split("\n")]
    log = open(os.path.splitext(path)[0] + ".log", "w", encoding="utf-8", newline="")
    m = MPE()
    out = []

    def say(t):
        out.append(t)
        log.write(t)
        log.flush()

    m.read(2)
    m.sendline("")
    say(m.expect(":", 20))
    m.sendline("HELLO MANAGER.SYS")
    say(m.expect("Reserved\\.\r\n:", 40))
    mode = "mpe"
    for n, raw in enumerate(lines):
        if not raw.strip():
            continue
        if raw.startswith("#SLEEP"):
            time.sleep(float(raw.split()[1]))
            say(m.read(2))
            continue
        if raw.startswith("#FEED "):
            try:
                m.expect(FEEDPROMPT, 120)
            except TimeoutError as e:
                say("\n*** no %s prompt before %r: %s\n" % (FEEDPROMPT, raw[6:], e))
                break
            m.sendline(raw[6:])
            continue
        if raw.startswith("#WAITPROMPT"):
            say(m.expect(BPROMPT, 300))
            continue
        if raw.startswith("#SEND "):
            m.sendline(raw[6:])
            time.sleep(0.05)
            continue
        if raw.startswith("#MPE"):
            mode = "mpe"
            continue
        if raw.startswith("#BASIC"):
            m.sendline("RUN BASIC")
            say(m.expect(BPROMPT, 60))
            mode = "basic"
            continue
        if raw.startswith("#"):
            continue
        if raw.strip() == "EXIT" and mode == "basic":
            m.sendline("EXIT")
            say(m.expect(MPROMPT, 60))
            mode = "mpe"
            continue
        # a RUN whose program reads the terminal must not wait for a prompt
        feeds = any(l.startswith("#SEND") or l.startswith("#FEED") for l in lines[n + 1:n + 3])
        m.sendline(raw)
        if feeds:
            say(m.read(1.0))
            continue
        try:
            say(m.expect(BPROMPT if mode == "basic" else MPROMPT, 300))
        except TimeoutError as e:
            say("\n*** TIMEOUT after %r: %s\n" % (raw, e))
            break
    if not keep:
        if mode == "basic":
            m.sendline("EXIT")
            say(m.read(3))
        m.sendline("BYE")
        say(m.read(2))
    m.close()
    log.close()
    print("".join(out))


if __name__ == "__main__":
    main(sys.argv[1], "--keep" in sys.argv)
