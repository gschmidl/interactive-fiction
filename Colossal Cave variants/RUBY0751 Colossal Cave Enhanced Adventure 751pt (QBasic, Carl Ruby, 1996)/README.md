# Colossal Cave Enhanced Adventure (Carl Ruby, 1996)

Carl Ruby played David Long's 751-point *Adventure* on CompuServe between 1982 and 1993, then rebuilt
what he had seen of it in Microsoft QBasic in the mid-1990s: 223 rooms by November 1996, with the
score ratings of his 350-point version. The program is unfinished (there is no endgame). This is his
program with 119 of his numbering slips fixed, compiled with QuickBASIC 4.5, for MS-DOS. It runs in
DOSBox; no emulator is included.

The game only understands capital letters. Nothing in Ruby's program tames the bear; the added debug
command `#BEAR` does.

## Command line

The DOS programs take no parameters. `HELLO.EXE` in `ADV751` starts the game: it offers the
instructions and runs `ADV751.EXE`. `SAVE` writes to `ADV751\GAMES`. `dosbox.conf` holds the
machine settings (S3 SVGA, 64 MB, 10,000 cycles); its start-up lines expect the folder that holds
`ADV751` to be drive C: already.

## Recommended start

In the unzipped folder:

`dosbox -c "mount c ." -c "c:" -c "cd \ADV751" -c "HELLO"`
