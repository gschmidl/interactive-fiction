「いろは歌の謎」 / Iroha Uta no Nazo
投稿: 北海道 笠原千秋 (CHIAKI KASAHARA)
Oh!MZ (MZ・2000 投稿), 1983年8月号, pp. 75-85

File: "Iroha Uta no Nazo.bas"  -- UTF-8, CRLF, 580 lines (10..5800)

Machine
-------
Sharp MZ-2000 / MZ-2200 with G-RAM 1,2.
For MZ-80B / MZ-80B2 the magazine says to change line 20 to:
    20 CLR:CONSOLE C80,S0,24 :TEMPO 6

Text encoding in this file
--------------------------
Program text is written with halfwidth (JIS X 0201) katakana, exactly the
characters the MZ prints.  The CHR$() codes used in the program are plain
JIS X 0201 / ASCII, e.g. CHR$(196)+CHR$(182)+... = ﾄｶﾅｸﾃｼｽ, so the katakana
here map 1:1 onto the machine's own character codes.

Three characters in this file stand for MZ cursor-control codes that the
magazine prints as inverse-video marks inside PRINT strings.  Replace each
with the matching control code (or PRINT CHR$(x)) when typing it in:

    ⓒ  U+24D2   clear screen / home   (printed as a circled cross)
    下  U+4E0B   cursor down
    右  U+53F3   cursor right

Everything else is literal: " , ; : ( ) < > [ ] * + - . ! ? and π (U+03C0,
the MZ's pi character, used in "FORJ=0TO2*π STEP.1").

Two lines have no closing quote; that is how the listing is printed and it
is legal BASIC:
    2040 PRINT"...ｵｵｸﾉ ﾀｲﾌﾟ ｶﾞ ｱﾙ｡
    5780 PRINT"...ｺﾌﾙｺﾉｺﾞﾛ｣.....

Line 4010 reads ｢ｳｲﾉｵｸｵﾔ｣ in the original (elsewhere ｢ｳｲﾉｵｸﾔﾏ｣); left as
printed.

Line 20 here is the MZ-80B/B2 form from the magazine footnote.  The MZ-2000
listing itself prints:  20 CLR:CONSOLE C80,S0,24,GN:TEMPO6

Author bug, transcribed as printed: line 5120 uses U$(40) where the poem wants
U$(30), so the screen shows ｱﾒｸﾓﾉ (あめくもの) instead of あまくもの (天雲の).
Verified against the scan - the magazine really prints 40.

Checks performed
----------------
* Line numbers 10..5800 in steps of 10, all present, no duplicates.
* Every GOTO / GOSUB / THEN target exists.
* All 50 DATA statements have exactly 32 values in 0..255, and rendering
  them as 16x16 bitmaps produces the 48 iroha kana in order plus 歌 and 謎.
* All 72 POSITION/PATTERN lines reference U$(1)..U$(50) only.
* All 19 CHR$() answer strings decode to sensible Japanese:
  ｶｷﾂﾊﾀ / ﾕｷ / ｹｻ / ｶﾏｸﾗｼﾞﾀﾞｲ / ﾖﾈﾀﾏﾍｾﾆﾓﾎｼ / ﾖﾈﾊﾅｼｾﾆｽｺｼ / ﾄｶﾅｸﾃｼｽ /
  ｵﾉﾉｵﾕ / ｲｵｳ / ﾓｼﾞｽｳ / ｳﾏｺ / 7 / ﾌｼﾞﾜﾗﾉﾌﾋﾄ / ﾓﾝﾑﾃﾝﾉｳ / 720 / ｼﾏﾈｹﾝ /
  ﾄﾑﾗｲﾉｳﾀ / ｵﾌｾﾖｴﾉｴｦﾅﾚｲﾃ / ｽｲｼ
