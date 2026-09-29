"""Minimal telnet driver for p50em / PRIMOS: wait for the system's own prompts, never for silence."""
import re, socket, sys, time

IAC, DONT, DO, WONT, WILL, SB, SE = 255, 254, 253, 252, 251, 250, 240


class Prime:
    def __init__(self, host="127.0.0.1", port=8720, log=None):
        self.s = socket.create_connection((host, port), timeout=10)
        self.s.settimeout(0.3)
        self.buf = ""
        self.log = open(log, "w", encoding="latin-1", newline="") if log else None

    def _negotiate(self, data):
        out = bytearray()
        text = bytearray()
        i = 0
        while i < len(data):
            b = data[i]
            if b == IAC and i + 1 < len(data):
                cmd = data[i + 1]
                if cmd in (DO, DONT, WILL, WONT) and i + 2 < len(data):
                    opt = data[i + 2]
                    if cmd == DO:
                        out += bytes([IAC, WONT, opt])
                    elif cmd == WILL:
                        out += bytes([IAC, DONT, opt])
                    i += 3
                    continue
                if cmd == SB:
                    j = data.find(bytes([IAC, SE]), i)
                    i = (j + 2) if j >= 0 else len(data)
                    continue
                i += 2
                continue
            text.append(b)
            i += 1
        if out:
            self.s.sendall(bytes(out))
        return text.decode("latin-1", "replace")

    def read(self):
        try:
            data = self.s.recv(4096)
        except socket.timeout:
            return ""
        if not data:
            raise EOFError("connection closed")
        t = self._negotiate(data)
        t = t.replace("\r\n", "\n").replace("\r", "")
        self.buf += t
        if self.log:
            self.log.write(t)
            self.log.flush()
        return t

    def expect(self, pattern, timeout=60):
        """Wait until pattern (regex) appears; return the text consumed up to and including it."""
        rx = re.compile(pattern, re.I)
        t0 = time.time()
        while True:
            m = rx.search(self.buf)
            if m:
                out, self.buf = self.buf[:m.end()], self.buf[m.end():]
                return out
            if time.time() - t0 > timeout:
                raise TimeoutError("waiting for %r; tail=%r" % (pattern, self.buf[-400:]))
            self.read()

    def send(self, text, delay=0.03):
        """Type text a character at a time (the AMLC line is slow to wake up)."""
        for ch in text:
            self.s.sendall(ch.encode("latin-1"))
            time.sleep(delay)

    def line(self, text, delay=0.03):
        self.send(text + "\r", delay)

    def cmd(self, text, prompt=r"\nOK, |\nER! ", timeout=120, delay=0.03):
        self.line(text, delay)
        return self.expect(prompt, timeout)

    def login(self, user="guest", pw="pr1me"):
        # The AMLC line prints its login banner only after the first carriage
        # return, and a line whose connection was dropped without LO stays
        # busy until PRIMOS reaps it, so keep knocking for a while.
        for _ in range(20):
            try:
                self.expect(r"login:", 8)
                break
            except TimeoutError:
                self.line("")
        else:
            self.expect(r"login:", 30)
        self.line(user)
        self.expect(r"password:", 30)
        self.line(pw)
        return self.expect(r"\nOK, ", 60)

    def logout(self):
        """Always log out: a dropped line stays busy until PRIMOS reaps it."""
        try:
            self.line("")
            self.line("LO")
            self.expect(r"logged out", 20)
        except Exception:
            pass

    def close(self):
        self.logout()
        try:
            self.s.close()
        except Exception:
            pass
        if self.log:
            self.log.close()


if __name__ == "__main__":
    p = Prime(log="primesh.log")
    print(p.login().strip()[-300:])
    for c in sys.argv[1:]:
        print("--- " + c)
        print(p.cmd(c)[:2000])
    p.close()
