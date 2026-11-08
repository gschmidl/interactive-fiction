"""Commodore BASIC V2 tokenizer (crunch rules of the C64 ROM) for byte strings already in PETSCII."""

KEYWORDS = [
    "END", "FOR", "NEXT", "DATA", "INPUT#", "INPUT", "DIM", "READ", "LET", "GOTO", "RUN", "IF",
    "RESTORE", "GOSUB", "RETURN", "REM", "STOP", "ON", "WAIT", "LOAD", "SAVE", "VERIFY", "DEF",
    "POKE", "PRINT#", "PRINT", "CONT", "LIST", "CLR", "CMD", "SYS", "OPEN", "CLOSE", "GET", "NEW",
    "TAB(", "TO", "FN", "SPC(", "THEN", "NOT", "STEP", "+", "-", "*", "/", "^", "AND", "OR", ">",
    "=", "<", "SGN", "INT", "ABS", "USR", "FRE", "POS", "SQR", "RND", "LOG", "EXP", "COS", "SIN",
    "TAN", "ATN", "PEEK", "LEN", "STR$", "VAL", "ASC", "CHR$", "LEFT$", "RIGHT$", "MID$", "GO",
]
KW = [(k.encode("ascii"), 0x80 + i) for i, k in enumerate(KEYWORDS)]
TOK_REM, TOK_DATA = 0x8F, 0x83


def crunch(text: bytes) -> bytes:
    """Tokenize one line body (PETSCII bytes, line number already removed)."""
    out = bytearray()
    i = 0
    quote = False
    raw_mode = None  # 'rem' or 'data'
    while i < len(text):
        c = text[i]
        if raw_mode == "rem":
            out.append(c)
            i += 1
            continue
        if c == 0x22:
            quote = not quote
            out.append(c)
            i += 1
            continue
        if quote:
            out.append(c)
            i += 1
            continue
        if raw_mode == "data":
            if c == ord(":"):
                raw_mode = None
            out.append(c)
            i += 1
            continue
        if c == ord("?"):
            out.append(0x99)
            i += 1
            continue
        if c >= 0x80 or c == 0x20 or (0x30 <= c <= 0x3B):
            out.append(c)
            i += 1
            continue
        for kw, tok in KW:
            if text.startswith(kw, i):
                out.append(tok)
                i += len(kw)
                if tok == TOK_REM:
                    raw_mode = "rem"
                elif tok == TOK_DATA:
                    raw_mode = "data"
                break
        else:
            out.append(c)
            i += 1
    return bytes(out)


def build_prg(lines, start=0x0801) -> bytes:
    """lines: list of (number, petscii body bytes). Returns PRG bytes with load address."""
    body = bytearray()
    addr = start
    for num, text in lines:
        tok = crunch(text)
        nxt = addr + 2 + 2 + len(tok) + 1
        body += bytes([nxt & 0xFF, nxt >> 8, num & 0xFF, num >> 8]) + tok + b"\0"
        addr = nxt
    body += b"\0\0"
    return bytes([start & 0xFF, start >> 8]) + bytes(body)
