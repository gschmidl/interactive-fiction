"""Drive a SINTRAN III terminal on RetroCore's TCP port (telnet-ish)."""
import socket, time, sys, re

class Term:
    def __init__(self, host='127.0.0.1', port=9000, log=None):
        self.s = socket.create_connection((host, port))
        self.s.settimeout(0.2)
        self.buf = bytearray()
        self.log = open(log, 'wb') if log else None
    def _pump(self, t):
        end = time.time() + t
        while time.time() < end:
            try:
                d = self.s.recv(65536)
                if not d: break
                # strip telnet IAC sequences
                out = bytearray(); i = 0
                while i < len(d):
                    c = d[i]
                    if c == 255 and i + 1 < len(d):
                        cmd = d[i+1]
                        if cmd in (251, 252, 253, 254) and i + 2 < len(d):
                            i += 3; continue
                        if cmd == 250:
                            j = d.find(bytes([255, 240]), i)
                            i = j + 2 if j >= 0 else len(d); continue
                        i += 2; continue
                    out.append(c); i += 1
                self.buf += out
                if self.log: self.log.write(out); self.log.flush()
            except socket.timeout:
                pass
    def expect(self, pat, timeout=30):
        rx = re.compile(pat.encode() if isinstance(pat, str) else pat)
        end = time.time() + timeout
        while time.time() < end:
            m = rx.search(bytes(b & 0x7f for b in self.buf))
            if m:
                txt = bytes(b & 0x7f for b in self.buf[:m.end()])
                del self.buf[:m.end()]
                return txt
            self._pump(0.2)
        raise TimeoutError('waiting for %r; have %r' % (pat, bytes(b & 0x7f for b in self.buf[-300:])))
    def send(self, s, cr=True):
        if isinstance(s, str): s = s.encode()
        self.s.sendall(s + (b'\r' if cr else b''))
    def drain(self, t=1.0):
        self._pump(t)
        txt = bytes(b & 0x7f for b in self.buf); self.buf.clear(); return txt

if __name__ == '__main__':
    t = Term()
    t.send(b'\x1b', cr=False)
    print(t.drain(3).decode('latin-1'))
