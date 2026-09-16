# Quest — what was worked out

Everything here was derived from the two programs and the AOS/VS `:UTIL`
sources (`PARU.32.SR`, `SYSID.32.SR`) that came with the Thissala export.
Both programs shipped their `.ST` linker symbol tables, so the code could be
read with the authors' own names on it rather than as addresses.

**PARU is assembled in octal.**  `EREOF= 30` is twenty-four.  Every number
taken from it must be read that way unless it carries a trailing decimal
point (`?IMPRT= 2047.`); the shim now writes them as C octal literals.

## The tape

`NADGUG_Library_1996-Jul-02.9trk` is a SIMH `.tap` image — 4-byte
little-endian record lengths, uniform 8192-byte records — carrying an AOS/VS
DUMP_II stream.  32 tape files, each one `<NAME>.DMP.Z`: a DUMP archive
compressed with Unix `compress`, which `gzip -dc` reads.  `tools/loadg.py`
unpacks both layers; `../tape/README.md` has the details.

A DUMP record header is one 16-bit word: 6-bit type, 10-bit length.  Types
are 0 SOD, 1 FSB, 2 name, 3 UDA, 4 ACL, 5 link, 6 start, 7 data, 8 end,
9 EOD.  A data block's header is byte address (4), byte length (4),
alignment count (2), then that many padding bytes, then the data.

`GAMES.DMP.Z` holds `:NADGUG:GAMES`, which is where Quest, Thissala, Zork and
Ferret all came from.  The only Colossal Cave on the tape is the 350-point
one already ported, and there is no BASIC anywhere on it.

## The two programs

Both are 32-bit MV programs (`USTPR` bit 15 clear) linked to a top of 1536
blocks, and both are FORTRAN 77 — `F77?INIT`, `I.START`, `I.HEAP`, `?BOMB`.

**The address space is bigger than the earlier games'.**  Zork and Ferret end
their shared half exactly at word `0x80000`; Quest runs to `0x180000`
(`USTST` 95 + `USTSZ` 1441 = 1536 blocks of 1024 words), and the shared
partition sits above that again, so `MEMWORDS` here is two megawords.

**They were linked for the SWAT debugger, which moves the start vector.**
The runtime's start-up is a block of long jumps in the UST extension that
begins `LJMP I.INIT` — at `?USTA`, word `0x126`, in Zork and Ferret, which is
why the loader used to read the start address from word `0x127`.  Quest's UST
carries two ring-3 pointers to `0x1AC` where Zork's has −1, the extension
holds `.SWAT.IPC` and `?000SWAT.TMP.BKPT` first, and the jump block has moved
to `0x1B8`; word `0x127` is zero.  Starting at the user main instead of
`I.INIT` ran surprisingly far — shared data, IPC, the whole logon — before the
first heap allocation, the stack for the first task, came back as address 1.
The loader now scans the extension for the first `LJMP` whose target begins
`WPSH 1,1`, which gives Zork's answer too.  With `I.INIT` running, each
program also looks for a SWAT debugger (`?ILKUP ?1.SWAT.IPC`), does not find
one and carries on.

## The shared world

`INIT_SHARED_DATA` reads the shared partition with `?GSHPT`, keeps
`(start + size) << 10` as the base of its own area in `SD_PTR`, and grows the
partition by 150 blocks with `?SSHPT`.  Both programs are linked to the same
top, so both get the same base and the two windows line up.

`?SOPEN` then opens `SHARED_DATA_FILE`, `WORLD_DATA_FILE` and
`CASTLE_DATA_FILE`, and `?GET_SHARED_PAGE` maps each with `?SPAGE`.
`SHARED_DATA_FILE` goes into the partition; `WORLD_DATA_FILE` (1100 pages)
and `CASTLE_DATA_FILE` go straight over the program's own F77 COMMON — words
`17C00..12AC00` of `QUEST.PR` are 1,126,400 words of zeros, exactly the 1100
pages it asks for.  Counts and record numbers in the `?SPAGE` packet are in
**512-byte blocks**, four to a page.

The three files are held whole in memory and every mapping is a window onto
one of them, synchronised at a process switch.  Under cooperative scheduling
that is exactly equivalent to real shared memory: the process that is not
running cannot observe a half-written page.  Two rules keep it so: a process
that ends has its mappings dropped after its last write-back, so its stale
window is never copied over later changes; and when the emulator stops, the
process that ran last — normally the server, tidying up after the player —
is written back before the files are flushed.

**After logon the game uses no IPC at all.**  Moving, fighting, choosing a
class — the client does all of it by writing the player's slot in the shared
data (`SD_PTR + 686*player`) and the world objects in the mapped COMMON.  The
server is involved only at logon, at character creation, and when a player
leaves.

## Calling conventions that are not the packet convention

Several AOS/VS calls take bare accumulators, and several return their answer
in AC1, not AC0.  The pattern is always visible in the caller: the wrapper
files AC0, AC1 and AC2 at FP+2, FP+4 and FP+6 after the call, and its success
path returns one of them.

| call | in | out |
|---|---|---|
| `?SOPEN` | AC0 byte pointer to the name, AC1 −1, AC2 channel wish | **AC1** = channel; AC0 = error code |
| `?ILKUP` | AC0 byte pointer to the port name | **AC1** = global port number |
| `?PNAME` | AC1 −1 for "me" | **AC1** = PID |
| `?SPAGE` | AC1 channel, AC2 packet | — |
| `?CON` | AC0 the server's PID | — |
| `?SCLOSE` | AC0 channel | — |
| `?REC` | AC0 mailbox address | **AC1** = message |
| `?XMT` | AC0 message, AC1 mailbox address | — |

## IPC

A global port number is `(pid << 16) | local port`, which is exactly how the
packets carry it — `?IDPH`/`?IDPL` and `?IOPH`/`?IOPL` are the high and low
words of one 32-bit number, `?IOPN`/`?IDPN` the local half alone.

The server registers the port name **QUEST** with `?SERVE` and leaves its PID
in the shared data at `SD_PTR+42`; `LOGON` looks the name up with `?ILKUP`,
reads the PID out of shared memory and `?CON`s to it, then talks to it with
`?IS.R`.  The server must be running before the player looks: the player's
process is held back until the server's first `?IREC`.

**A zero-length message carries its payload in the header.**  `IPC_TASK` in
`QUEST_SERVER` builds its reply by storing into `?IUFL` and `?IPTR` and
setting `?ILTH` to nought (17B23D–17B241), and `LOGON` reads the answer back
out of `?IPTR` (175F3D).  Logon status 1 is a good login, 3 is *"This player
was previously killed off!"*.

**The customer's obituary.**  When a player's process ends, AOS/VS tells the
server it was connected to.  `IPC_TASK` checks for a message from origin port
8 (17A669) and hands the low byte of `?IUFL` — the dead process's PID — to
`HANDLE_TERM`, which finds the player's slot by that PID, copies the slot into
the player's `USER_DATA_FILE` record, writes it back and frees the slot.  The
emulator sends exactly that when the player leaves, by ESC or by the end of a
script, and then lets the server run until it is idle.

## Tasks and waiting

`MT?TASK`, `MT?REC`, `MT?XMT` … in the program's own runtime say what each
call takes.  All of Quest's tasks start at one trampoline: `MT?TASK` puts
`17E784` in `?DPC` and the routine the caller asked for in `?DAC2`.

A blocked task resumes by **re-executing** its system call: it records what it
waits for, the gate winds the PC back to the `LCALL`, and when the task runs
again the call happens a second time.  That is right for waits that test a
state — `?IREC`, `?REC`, the keyboard — but wrong for the ones that wait for
an event, and each of those needed its own marker:

* **`?IS.R` must not send twice.**  Using the wait state as the "already
  sent" marker failed because the wake-up clears it; every request went out
  twice, the server took the second LOGON from the same port as a second
  player, and answered the password check with "previously killed off" for
  every character there was.  The marker is a flag of its own now.
* **`?WDELAY`, `?SUS` and `?WTSIG` complete when woken.**  Without that a
  delay slept for ever — each re-run was the same delay starting again.
* **The instruction clock is 64 bits.**  Delays wait on it, and `long` is 32
  bits on Windows: the server alone spends ninety million instructions
  building its world, and the counter wrapped within minutes of play.

The keyboard is the one wait that stops the whole machine, so a task that
wants a key waits until nothing else anywhere can run.  When nothing can run
and nobody wants a key, the clock is moved on to the earliest delay.  On a
console, delays are real pauses instead.

## The D200

`?READ_SCREEN` and `?WRITE_SCREEN` build extended I/O packets: `?ISTI` has
`?IPKL`, and `?ETSP` (packet+16) points at a three-word screen-management
packet — `?ESFC` flags, `?ESEP` edit position, `?ESCR` <column><row>.  The
flags that matter: `?ESCP` 0x0800 put the cursor at `?ESCR` first, `?ESRP`
0x0200 hand the position back, `?ESNE` 0x0100 no echo, `?ESED` 0x1000 no
echo for the delimiter.  Commands are **binary** reads (`?IBIN`) of one byte.

The codes the game writes: 020 col row cursor address, 012 new line, 013
erase to end of line, 014 erase screen, 027/030/031/032 cursor movement, and
the attributes 024/025 underscore (the player's initials) and 034/035 dim
(terrain).  Map cells are four bytes each — attribute on, two characters,
attribute off — taken from a symbol table the server writes into the shared
data at startup; in the tape's `SHARED_DATA_FILE` that table is zeros.

## The emulator bugs this game found

**`WMSP` allocates doublewords, not words.**  Every caller computes its
argument as `(bytes + 1 + 3) >> 2`.  `?OPEN_SHARED_IO_FILE` asks for 5 and
copies eighteen bytes into what it got, so a word-sized `WMSP` left it ten
bytes short and the `?SOPEN` that followed pushed its return block over the
middle of the name.  **ZORK.PR and FERRET.PR build stack buffers the same
way** and were under-allocating too; their copies of the emulator were not
changed, because they work and were not retested.

**`XVCT` and `QSCAN` in their `CF..` form are one word, not two.**  They are
the traps the F77 compiler plants after a range check, and MASM's table type
05 implies an operand word that is not there.  Read as two words, the skip
over the trap landed one word late and the machine executed an operand.  Only
`0xCF09`/`0xCF19` occur as instructions (75 and 5 here, 67 in `DISCO.PR`);
the `C7..` forms keep MASM's length, which Zork's `0xC719` allocator walk
needs.

**`WMESS` is the release half of `I.LOCK`**, not a no-op.  `I.LOCK` takes the
lock with `WSZBO 2,1` (bit 32 of the lock = bit 15 of the word two on) and
waits with `?REC` on the lock's address when it is already set; `I.UNLOCK`
points AC2 at the flag word and executes `WMESS`, which clears the flag and
posts to that mailbox.  `INIT_SCREEN` locks the display twice, and with the
unlock doing nothing the second lock waited for ever.

**After `WCMV`, AC1 is the number of source characters not moved** — the
Eclipse `CMV` definition — not nought.  `DISPLAY_SCREEN` appends each map row
with `AC0 = row end, AC1 = 756 / WCMV / … / NLDAI 0,756 / WSUB 1,0` and takes
the new length as 756 minus AC1.  With AC1 zeroed the map was "full" after
its first row: rows two to nine were never drawn, and the name under the map
came out with binary after it.

**`mvdis.py` now decodes exactly as the emulator does** — `WBR` with its
target, the one-word traps, `WCLM`, `WSKBO/WSKBZ` — and prints six-digit
addresses.  The five-digit ones had turned `15BC75` into `5BC75`.

## Rules in the shim that were wrong for a game with its own data files

* **`?CREATE` refuses a file that came with the game.**  The server calls it
  on `USER_DATA_FILE` at startup and ignores the error; creating it regardless
  left an empty file and the next read sent the server into `?FATAL`.  The
  other `?CREATE`, `?CREATE_IPC_FILE`, makes a transient file and must work.
* **A read off the end says `EREOF`** — 030 — and the server compares with
  exactly that to find the end of `USER_DATA_FILE`.
* **An `?OPEN` for update works on a copy of the original**, never on a new
  empty file.
* **A file read and write must be separated by a reposition.**  C stdio
  forbids a write straight after a read on the same stream, and the Windows
  runtime quietly loses it.  A relative "next record" write makes no `fseek`
  of its own — and that is precisely the save: `UPDATE_USER_DATA_FILE` reads
  its way to the player's record and writes in the next breath.  Every
  character used to come back exactly as it was created.
* **The server's console is `QUEST.OUT`.**

## What is left

* Only one player.  The design allows more — each would be another `QUEST`
  process connected to the same server — and nothing here prevents it except
  that there is one keyboard.
* `?UPDATE`, `?RECREATE`, `?DCON`, `?DEBUG` and `?INTWT` are implemented
  thinly.  `?INTWT` never fires: there is no console interrupt (the D200's
  CTRL-C CTRL-A).
* Ctrl-C in the console kills the emulator without writing the world back;
  leave with ESC.
* Zork's and Ferret's emulators still carry the `WMSP`, `WCMV` and stdio
  findings unfixed.

## Later: shared with the 1984 Quest (2026-09-16)

`src32/` is now the same emulator as the port of the earlier Quest from
`AOS-VS_QUEST_game__1984.9trk` (`../../Quest 1984 (Data General MV,
AOS-VS)/port`, whose `NOTES.md` has the details).  What that build added:
LNDO/LWDO (four words, not the three the MASM-derived table implied); the DO
loop instructions now store the stepped index on the way out of the loop as
well, as the *Principles of Operation* says; 103750 executed as `FRDS 0,0`
(the older runtime's encoding in `SQR31?3`); and `-c <file>` to type a file
on the D200 first.  This port's four recorded checks are unchanged.
