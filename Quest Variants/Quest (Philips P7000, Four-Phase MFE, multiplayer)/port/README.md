# QUEST UNDER MFE - port

QUEST version 1, the multi-player cave game, as a Danish site ran it under MFE/7000 on a Philips P7000, which is a
Four-Phase Systems IV/90. The game is not rewritten. The port boots the site's own disc pack
(`../src_original/P7000.PACK`) on an emulated Four-Phase IV/90 Model 2 and does what the operator did: it starts MFE
from IDOS, gives MFE the time and the date, starts QUEST and tells it how many players there will be. Each player
then plays at a 7200 terminal of their own: terminal 0 in the first window, terminals 1 to 5 in windows that join it.

## Build and play

    build.bat                  (or sh build.sh; needs MinGW-w64 gcc and sh on PATH - Windows only)
    run.bat                    one player
    run.bat --players=3       a game for three: start run.bat in two more windows and they join it

`build.sh` compiles `quest.exe` and copies the pack to `p7000.pack` and QUEST's manual to `QHELP.txt`. `run.bat`
runs `quest -u`. The pack is only read: QUEST keeps nothing between games (each game builds a new world).

The first window shows the machine starting: IDOS, then MFE's console as the operator answers it. Then QUEST's
sign-on comes up at terminal 0 (the port types the Q that signs a player on). Give your name and answer "Are you
male?"; the game begins when every player has done so. Then type sentences and press Enter. `QHELP.txt`, QUEST's
own manual, explains the game; HELP in the game gives the short version.

- **Enter** gives QUEST the line. **Backspace**, **Left**, **Right**, **Home**, **Insert** and **Delete** edit it
  (Shift+Left deletes and Shift+Right inserts, as on the 7200).
- **Esc** blanks the line: the 7200's MODE (ATTN) key. At terminal 0 that key is MFE's System Console key, so there
  the port blanks the line with HOME and DELETEs instead.
- **Ctrl+Enter** logs you off (Control CURSOR RETURN; QUEST asks, as for QUIT).
- **Ctrl+C** closes the window. In the first window it ends the game for everyone.

**Several players.** The first window runs the machine; a game for more than one listens on TCP port 7000 (on
127.0.0.1; with `--lan` on every interface, for IPv6 as well as IPv4). A window started while a game waits for
players finds the port taken and joins that game. On another computer use `quest --join=HOST`: with `--lan` the first
window shows this computer's name to give it (an address works too; `[ADDRESS]:PORT` for IPv6). Windows asks the
first time whether quest.exe may take connections from the network. A window that closes logs its player off. QUEST
stops when its last player has logged off, and the port ends then.

    quest [OPTION]...
      -p, --players=N       the number of players, 1 to 6 (default 1)
      -u, --unlimited       open the cave at any time: give the site's password when QUEST asks
          --easy            play with the 'EASY' library (the site had the 'HARD' one installed)
          --port=N          the TCP port for the players' windows (default 7000)
          --lan             let players on other computers join (the window shows the name to give --join)
          --join=HOST[:PORT]  be a player in the game started on computer HOST, a name or an address
                            ([ADDRESS]:PORT for IPv6)
          --transcript      follow terminal 0 as a log and read its lines from stdin
                            (the default when stdin or stdout is not a console)
          --fixed-clock     the 60 Hz clock counts instructions, and MFE gets a fixed time and date
                            (12:00 on 9 April 1980, the date of the tape): a run repeats exactly
          --trace=FILE      an instruction trace (debugging)
      -h, --help

Unknown options and bad values are refused with exit status 2. There are no fixes, so there is no `--no-fixes`.

## About this version

- **QUEST** is a Pascal program for MFE on the IV/90 Model 2 ("Quest requires MFE release BN03, configured for 24 x
  81 screens"). Almost everything about the cave, the quest included, is drawn at random when the game starts, and
  actions take real time. Nothing on the pack dates it (the "7 June 1978" of the VCFed thread is ADVENT's banner).
- **The hours.** QUEST does not run on weekdays from 9 to 12 and from 13 to 17. It then says "SORRY -- THE CAVE IS
  CLOSED NOW." on the MFE console and asks for a password, which sits in QUEST's code after the text "NOTE TO SE:
  USE CRTDMP TO CHANGE PASSWORD:" - the site's is `VISION S@`. With `-u` the operator gives it. Without `-u` the
  operator gives none, QUEST stops, and the port says the cave is closed and exits with status 1. QUEST's calendar
  takes the year 26 as 2026 (it closes on the weekdays of September 2026, not those of 1926).
- **The libraries.** QUEST reads its data from the file QLIB. The pack holds three: QLIB and QLHRDS carry the same
  "THIS IS THE 'HARD' QUEST VERSION 1 LIBRARY", QLIBSV the 'EASY' one QUEST came with ("If a harder game is desired,
  the file QLHARD should be copied to file name QLIB"). The site had installed the hard one, and the port keeps it.
  `--easy` puts QLIBSV's data into QLIB, in memory only.
- **Six terminals.** The site's MFE drives screens 0 to 5, so six players at most (QUEST itself allows twenty, and
  answers a larger number with "YOU ARE NOT EVEN CONFIGURED FOR THAT MANY TERMINALS").

## How it works

- `src/cpu.c` is the IV/70 processor with the IV/90 Model 2's additions: the memory mapper (256 windows of 32 pages
  of 1K words; 128 pages of memory), MAP, MVEL, IOXW, BDEC, DBIN and BYTE. Their behaviour was inferred from IDOS,
  MFE and QUEST (`_FourPhase_work/docs/ISA_NOTES.md`, "IV/90 behaviour inferred"). BIT is not emulated; nothing here
  uses it.
- `src/io.c` holds the devices: the 8231 disc (the pack in memory, never written back); the 60 Hz clock, in real
  time or counting instructions; the 7200 keyboards on channel 3. MFE's keyboard alarm (the beep for typing too
  fast) rings the player's window.
- `src/quest.c` is the front end:
  - **The operator.** At IDOS's `// $BATCH` it types the site's job, `// MFE` `/C=DAMHUS` `//`. At "MFE/7000 IS
    READY" it presses the System Console key (0205). It answers the hour, the minute and the date (DDMMYY), types
    `START,QUEST`, answers the password question (with `-u`) and the number of players, and presses the System
    Console key again. MFE's console shows RSP when a program waits for an answer and MSG when more messages are
    queued (Arrow Up shows the next); both blink. After the password, QUEST's question waits behind MSG. The
    operator answers a question only when it has been on the screen for half a second: a key typed while QUEST is
    still printing goes to MFE's command line.
  - **The screens.** Terminal k's screen is 24 lines of 32 words (81 characters) at 0140 of physical page k. The
    site's 7200s use the "300" attributes: a byte 0300-0377 shows as a blank and sets the look of what follows
    (QUEST starts every line with 0310, bright). 032 is the cursor, which MFE blinks by putting it in and taking it
    out. The port draws each screen as ANSI text, in its own console or over the player's connection.
  - **The players.** The port types Q at a terminal when a player is there, and the player answers QUEST. A terminal
    that is back at MFE's screen for half a second has lost its player. When a player's window closes, the port
    blanks the line (MODE), types Control CURSOR RETURN, and types YES only once QUEST's "Do you really want to quit
    the game?" has appeared as a new line (typed ahead of the question, the YES was sometimes lost); after five
    seconds with no question it starts again.
  - **The transcript.** QUEST scrolls its display (lines 0-22) with one MVEL from line 1 to line 0 and clears it with
    another; MFE puts its own screen back with MVELs when a player leaves. The CPU reports every MVEL into a
    terminal's display. The log takes each line as it scrolls away, and everything not yet taken before a clear
    (columns 1-80: column 0 holds the attribute). A line from stdin is typed when terminal 0's input line has been
    empty for two seconds of game time. A piped stdin is read without waiting, so the others play on meanwhile.
    QUEST holds one line per player. Until it has taken the last one (an action takes time), it refuses every key
    with a beep and shows none of it, so a line of which nothing showed is typed again, as a player would.
  - **Idling.** MFE/7000 BN03-C waits for work in a loop at 06004-06012 of window 020. When a quarter of a slice of
    8192 instructions ran there, the port sleeps (1 ms, or until a key or a network message comes). In a game,
    QUEST's own main loop never stops looking for work, so with the real clock the machine then gets at most 32,000
    instructions a clock tick - four times what `--fixed-clock` gives it, under which every test and fuzz game
    runs. Measured: 4.5% of a host core while a player types a name, 13% in a game (97% and 62% without the two).
    With six players typing at once, 10-11%, and the machine never more than a tick behind the clock.
- `src/net.c` has the players' windows (TCP), the console (ANSI output, raw key events) and Ctrl+C.

What QUEST showed about the machine, on top of what running MFE had shown:
1. **The keyboards.** MFE takes every key with a status and a data-in on unit 0; the status word names the keyboard
   (0200 + k), and the data-in gives that keyboard's key. The unit in the IOID interrupt only picks the handler.
2. **Control CURSOR RETURN is 0376**, and QUEST asks "Do you really want to quit the game?" for it, as for QUIT. The
   System Console key is 0205, ATTN in the reference summary.
3. **The console blinks** its RSP and MSG indicators and the cursor.
4. **An IO at an odd address** (found 2026-09-22 by the fuzz below). QUEST keeps a 075-word block for each terminal,
   at 010501 + 075 t, and in it `IO X+014`. Its select word at X+014 is a control IO to the terminal's keyboard
   unit, the bell. QUEST's key routine (037547) runs it, and drops the key, when a key comes while QUEST still
   holds the player's last line: the beep for typing ahead. The manual wants an IO's address even, with the buffer
   address word at "the address ORed with 1",
   but the blocks of terminals 0, 2 and 4 start at odd addresses. Read the manual's way, their IO took a counter
   (X+013) for the select word and the select word for the buffer address. At sign-on that looked like harmless
   output to a unit 015. In play it became status and data-in IOs that wrote zeros through MFE's code, and MFE
   halted (HLT at 01413, 01421, 01753, ...) a minute into a real-clock game with three players. Now the select
   word is taken from the address and the buffer word from the next one, which is what QUEST's layout needs; for
   an even address nothing changes. The beeps now ring the player's window.
5. **The other 7200 keys** (checked 2026-09-22). EOM, shifted EOM and shifted CURSOR RETURN end the line as CURSOR
   RETURN does; ERASE puts the cursor at the left margin as HOME does; ROLL up and down, TAB, vertical TAB, the
   function keys F1-F11, TOTAL and the control keys (Control 0-9, Control -, Control ROLL, Control EOM, Control HOME,
   Control TAB) do nothing at all. So the port maps none of them: Enter and Home already do what the others do.

## Tests

    python tests/regress.py            options; the reference walk, twice; the closed cave with and without -u,
                                       and on a Saturday; --easy; the end of stdin
    python tests/regress.py --record   rewrite tests/reference/walk.out from this build
    python tests/netplay.py            three players over TCP: telnet line ends, a refused fourth, speech, a
                                       window that closes, Control CURSOR RETURN, the end
    python tests/consoleplay.py        at a real console (ConPTY): one player, Backspace, Esc, QUIT, Ctrl+C,
                                       two windows playing together, run.bat
    python tests/lanplay.py            a --lan game for four: the first window names this computer; a second
                                       game finds the port taken; players join by the computer's name (IPv6
                                       first), by [::1]:PORT, and from WSL2's virtual machine over IPv4 (left
                                       out without WSL); they see and hear each other and leave in three ways
    python tests/fuzz.py [GAMES] [TURNS] [SEED] [--real-clock]   random play; run it from a scratch folder
                                       (--real-clock: with -u on the real clock and the speed cap, as run.bat
                                       plays; 4 games of 20 lines clean on 2026-09-22)
    python tests/netfuzz.py [GAMES [PLAYERS [SECONDS [SEED]]]] [--real-clock]
                                       random commands from every terminal at once (terminal 0 scripted, the
                                       others over TCP), every half to two seconds, then QUIT all round; the
                                       machine must not stop and the port must end with exit 0 (2026-09-22,
                                       after the odd-address fix: 2 games of 3 and 2 of 6 players with
                                       --real-clock clean; 200 beeps a game with 3 players, 500 with 6, for
                                       keys typed ahead)

`tests/reference/walk.out` is this port's own output: there is no other P7000 to compare with. It runs with
`--fixed-clock`, so it repeats exactly; with the real clock the world differs every time.

Environment variables for tests and debugging: `QUEST_TIME=HH,MM,DDMMYY` (the time and date the operator gives MFE),
`QUEST_STOP_AT=N` (stop at instruction N and print every screen), `QUEST_TRACE_FROM=N` (start `--trace` there),
`QUEST_NETLOG=1` (log the players' connections and log-offs on stderr; netplay.py prints it when a test fails),
`QUEST_SMC=FILE` (from the game's start, report every write over a word that has run as an instruction, with the
last instructions when an IO writes it) and with it `QUEST_WATCH=WIN:ADDR,...` (octal; report every write to those
words).

## Still to do (refine pass)
- Nothing that can be done here. `--lan` was checked on 2026-09-22 with WSL2's virtual machine as the other
  computer (tests/lanplay.py); the user has no second computer, so `quest --join` has not run on one.
  That check found and fixed three things:
  - `--lan` listened for IPv4 only, while a computer's name resolves to IPv6 addresses first. On a real network the
    firewall would drop those connections, and `--join=NAME` would wait some 20 seconds for each before it tried
    IPv4.
  - `--join` could not take an IPv6 address.
  - The window gave no hint of what to join.
- Decided not to do (user, 2026-09-22): checking the console at other window sizes and in Windows Terminal.
