# Checking the transcription on a real BASIC/3000

These are the scripts that put the transcribed listing into a real HP 3000 and
recorded what it did, so the port could be compared against it rather than
against my expectations.

The machine is Gavin Scott's "My Big Series 58" turnkey distribution: J. David
Bryan's SIMH HP 3000 simulator running MPE V/E G.40.00, with BASIC/3000
HP32101B.00.26. The simulator is not in this repository. Start it so that one
ADCC line is attached to a TCP port (1044 in these scripts), then set
`HP3000_TOOLS` to the directory holding `mpe.py`, the small expect-style telnet
driver these scripts import.

| script | what it does |
| --- | --- |
| `basic_session.py` | types a script into MPE/BASIC line by line, waiting for each prompt (the terminal driver has no type-ahead, so anything typed early is simply lost) |
| `make_loaders.py` | writes the BASIC loaders that create the four data files on the machine; the movement table goes in as `'nn` character constants because its records are binary |
| `verify_data.py` | dumps every record's length and a position-weighted checksum from the machine and compares them with the transcription |
| `gen_program.py` | turns the transcribed program into the lines to type, breaking any statement longer than the interpreter's 131-character input buffer with `&` continuations |
| `play.py` | plays a list of moves and saves the transcript |
| `compare.py` | normalises a machine transcript and the port's output and diffs them |

`moves_*.txt` are the walkthroughs; `reference_*.txt` are the transcripts the
machine produced for them. The port replays the same moves in
`../port/tests/`, which is where the comparison runs as a regression test.

Results of this pass: all 1,003 statements were accepted with no syntax error,
the four data files matched the transcription record for record, and the
recorded walkthroughs replay in the port identically, line for line.

Note that the walkthroughs run copies of the program with `RND(0)` replaced by
`.5`, so both sides take the same branches; without that the game picks its
starting room at random on its very first line. `ADV3000T` is the program with
the author's three bugs corrected (walkthroughs a-d); `ADV3000V` has every
correction in `../transcription/fixes.txt`, made on the machine by retyping the
changed lines (walkthrough e).
