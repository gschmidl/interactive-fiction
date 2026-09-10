"""Drive an HP 1000 game running under SIMH through its Telnet console.

SIMH refuses to start when its command console is not a real terminal, so the
simulator is launched in its own window (see runsim.ps1 / the shell command in
the port README) with `set console telnet=<port>`; this script then plays the
emulated machine's console over that socket.

  python simdrv.py <port> <script-file> [outfile]

The script file is one input line per line.  A line of the form

    @wait <seconds>

pauses without sending anything, which is how you let RTE finish booting or
let a slow initialisation run.  Everything the machine prints is echoed to
stdout and, if given, appended to outfile.
"""
import socket
import sys
import time

IAC, DONT, DO, WONT, WILL, SB, SE = 255, 254, 253, 252, 251, 250, 240


def negotiate(sock, data, out):
    """Strip Telnet option negotiation, answering everything with a refusal."""
    i = 0
    while i < len(data):
        c = data[i]
        if c != IAC:
            out.append(c)
            i += 1
            continue
        if i + 1 >= len(data):
            break
        cmd = data[i + 1]
        if cmd in (DO, DONT, WILL, WONT):
            opt = data[i + 2] if i + 2 < len(data) else 0
            reply = WONT if cmd in (DO, DONT) else DONT
            sock.sendall(bytes([IAC, reply, opt]))
            i += 3
        elif cmd == SB:
            j = data.find(bytes([IAC, SE]), i)
            i = len(data) if j < 0 else j + 2
        elif cmd == IAC:
            out.append(IAC)
            i += 2
        else:
            i += 2
    return out


def drain(sock, seconds, sink):
    """Read whatever arrives for `seconds`, echoing it."""
    end = time.time() + seconds
    sock.settimeout(0.4)
    while time.time() < end:
        try:
            data = sock.recv(4096)
        except socket.timeout:
            continue
        except OSError:
            break
        if not data:
            break
        text = bytes(negotiate(sock, data, bytearray())).decode('latin1')
        text = text.replace(chr(0), '')
        sys.stdout.write(text)
        sys.stdout.flush()
        sink.append(text)


def main():
    port = int(sys.argv[1])
    lines = open(sys.argv[2], encoding='latin1').read().split(chr(10))
    outfile = sys.argv[3] if len(sys.argv) > 3 else None

    sock = socket.create_connection(('127.0.0.1', port), timeout=10)
    sink = []
    drain(sock, 3.0, sink)
    for line in lines:
        if line.startswith('@wait'):
            drain(sock, float(line.split()[1]), sink)
            continue
        if line.startswith('@@'):          # comment
            continue
        sock.sendall((line + chr(13)).encode('latin1'))
        drain(sock, 1.5, sink)
    drain(sock, 2.0, sink)
    sock.close()
    if outfile:
        open(outfile, 'w', encoding='latin1',
             newline=chr(10)).write(''.join(sink))


if __name__ == '__main__':
    main()
