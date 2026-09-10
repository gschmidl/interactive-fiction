# Alternative: a hand-written reimplementation

`misty_marsh_windows.html` is a self-contained browser version of Misty Marsh,
written from scratch in JavaScript. The room text was transcribed from the disk,
but none of the original's own logic runs — the branching, scoring and inventory
are all newly written, and the wording of anything the game *computes* rather
than prints verbatim is a reconstruction.

It is kept because it plays, and it needs nothing but a browser.

It is **not** the port. See `../README.md`: the real one runs the game's own
Wang word-processing glossary, which is the form the game was actually written
in, out of `src_original/mistymarsh.img` unmodified.
