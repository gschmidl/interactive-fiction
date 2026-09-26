# The NADGUG library tape

`NADGUG_Library_1996-Jul-02.9trk` is the North American Data General Users
Group library tape, dumped 15 September 1995.  Quest came off it, and so did
Thissala, Zork and Ferret — `:NADGUG:GAMES` is the whole games collection.
`CONTENTS` is the tape's own index: a description of each of the 32 archives
and a full directory listing of every file on it.

Two layers:

1. A SIMH `.tap` image — 4-byte little-endian record lengths either side of
   each record, uniform 8192 bytes — carrying an AOS/VS DUMP_II stream.
   32 tape files, each holding one `<NAME>.DMP.Z`.
2. `.DMP.Z` is a DUMP archive compressed with Unix `compress`; `gzip -dc`
   reads it.

`../port/tools/loadg.py` unpacks both:

    python ../port/tools/loadg.py NADGUG_Library_1996-Jul-02.9trk out
    gzip -dc out/tf15/GAMES.DMP.Z > games.dmp

and then `extract()` from the same file reads `games.dmp`.  The DUMP record
header is one 16-bit word, 6-bit type and 10-bit length; a data block's own
header is byte address (4), byte length (4), alignment count (2), that many
padding bytes, then the data.

Worth knowing: the only Colossal Cave on this tape is `:NADGUG:GAMES:ADVENTURE`,
which is byte-for-byte the 350-point AOS/VS game already ported.  There is no
BASIC of any kind anywhere on the tape.
