# メモ殺人 (MEMO ｻﾂｼﾞﾝ): walkthrough

T. Shimokawa, I/O magazine type-in, Sharp MZ-80K. TRS has been murdered, and the memo he left names the killer. It is hidden somewhere in his house, and you have 60 minutes of game clock to find it.

**The hiding place is random.** At start-up, line 8020 picks one of 42 hiding places. No single fixed route can work: searching everywhere would cost 82 minutes, because every wrong search adds 2 minutes to the clock. The game does give you a clue, though, and this walkthrough turns that clue into a list of keys to type.

## 1. Start the game and read the clue

1. At `ｾﾂﾒｲ ｲﾘﾏｽｶ ..? [Y/N]`, press **N**. You can press Y (the **Z** keycap on US layout) instead to read the story and the command list first; you end up in the same place.
2. The next screen shows TRS's room. **Two 2-digit numbers** are written on its back wall, in row 6 around the middle of the screen (line 9330 `CURSOR20,6:PRINTAN$`). Note them down, for example `72 89`.
3. At `START ?  [S] OR [H]  KEY`, press **H**. This skips the front door, and the clock starts. You are now in the entrance room, facing the open doorway (line 280), and every script starts from here.
4. **Switch the MZ-80K to kana mode** (section 3), and stay in it for the rest of the game.

If H does nothing, press **S** instead, switch to kana mode, and type the lines below. They open the front door and walk in, which leaves you in the same place.

```
3 Ä .    ｱｹﾙ
S < 3    ﾄﾞｱ
3 . H    ｱﾙｸ
```

### What the numbers mean

Lines 8040-8070 take two neighbouring characters from the hiding place's name, for example ﾎﾟ from ﾎﾟｽﾀｰ. They subtract one random number from 95 to 105 from both character codes and print the results. The random part cancels out if you subtract: **second number minus first number** depends only on the two characters. The first number then narrows it down further. The END screen confirms your answer: `72 89 ﾊ ﾎﾟｽﾀｰ ﾃﾞｼﾀ｡`

## 2. Look up your script

Work out **second − first**, find that row, and then pick the range your **first** number falls in.

| second − first | first number | script |
|---:|---|---|
| -47 | 85–95 | [ﾎﾞｯｸｽ (chest) / ﾍﾞｯﾄﾞ (bed)](#script-chest-bed) |
| -43 | 85–95 | [ﾚｲｿﾞｳｺ (fridge)](#script-fridge) |
| -40 | 81–91 | [ﾚｲｿﾞｳｺ (fridge)](#script-fridge) |
| -38 | 84–85 | [ｶﾝｷｾﾝ (vent fan)](#script-vent-fan) |
|  | 86–94 | [ﾎﾟｹｯﾄ (pocket) / ｶﾝｷｾﾝ (vent fan)](#script-pocket-vent-fan) |
|  | 95–96 | [ﾎﾟｹｯﾄ (pocket)](#script-pocket) |
| -37 | 85–95 | [ﾊﾞｹﾂ (bucket)](#script-bucket) |
| -34 | 86–96 | [ﾎﾟｽﾀｰ (poster)](#script-poster) |
| -27 | 69–79 | [ﾎｳﾁｮｳ (kitchen knife)](#script-kitchen-knife) |
| -20 | 66 | [ﾋｷﾀﾞｼ (drawer)](#script-drawer) |
|  | 67–76 | [ﾌｸ (clothes) / ﾋｷﾀﾞｼ (drawer)](#script-clothes-drawer) |
|  | 77 | [ﾌｸ (clothes)](#script-clothes) |
| -19 | 56–66 | [ﾎｳﾁｮｳ (kitchen knife)](#script-kitchen-knife) |
| -16 | 65–75 | [ﾊｺ (box)](#script-box) |
| -11 | 49–59 | [ｺｯﾌﾟ (cup)](#script-cup) |
|  | 70–80 | [ﾏﾄﾞ (window)](#script-window) |
| -10 | 48–56 | [ﾎﾟｹｯﾄ (pocket)](#script-pocket) |
|  | 57–58 | [ﾎﾟｹｯﾄ (pocket) / ﾂｸｴ (table)](#script-pocket-table) |
|  | 59–67 | [ﾂｸｴ (table)](#script-table) |
| -8 | 67–77 | [ﾌﾄﾝ (futon)](#script-futon) |
| -6 | 52–62 | [ｳｲｽｷｰ (whisky)](#script-whisky) |
| -1 | 42–52 | [ｳｲｽｷｰ (whisky)](#script-whisky) |
| 3 | 52–62 | [ﾎﾟｽﾀｰ (poster)](#script-poster) |
| 7 | 46–56 | [ｶﾝｷｾﾝ (vent fan)](#script-vent-fan) |
| 9 | 38–45 | [ﾎﾞｯｸｽ (chest)](#script-chest) |
|  | 46–48 | [ﾎﾞｯｸｽ (chest) / ﾋｷﾀﾞｼ (drawer)](#script-chest-drawer) |
|  | 49–56 | [ﾋｷﾀﾞｼ (drawer)](#script-drawer) |
| 11 | 41–51 | [ｲｽ (chair) / ｳｲｽｷｰ (whisky)](#script-chair-whisky) |
| 13 | 41–51 | [ﾚｲｿﾞｳｺ (fridge)](#script-fridge) |
| 14 | 42–52 | [ﾎｳﾁｮｳ (kitchen knife)](#script-kitchen-knife) |
| 15 | 69–79 | [ﾎﾝ (books)](#script-books) |
| 16 | 69–79 | [ﾎﾞｯｸｽ (chest)](#script-chest) |
| 17 | 68 | [ﾍﾞｯﾄﾞ (bed)](#script-bed) |
|  | 69–78 | [ﾎﾟｽﾀｰ (poster) / ﾍﾞｯﾄﾞ (bed) / ﾎﾟｹｯﾄ (pocket)](#script-poster-bed-pocket) |
|  | 79 | [ﾎﾟｽﾀｰ (poster) / ﾎﾟｹｯﾄ (pocket)](#script-poster-pocket) |
| 19 | 66–76 | [ﾋﾞﾝ (bottle)](#script-bottle) |
| 20 | 65–75 | [ﾊﾞｹﾂ (bucket)](#script-bucket) |
| 21 | 38–48 | [ﾍﾞｯﾄﾞ (bed)](#script-bed) |
| 28 | 50–60 | [ｻﾗ (plates)](#script-plates) |
| 29 | 38–48 | [ｺｯﾌﾟ (cup)](#script-cup) |
| 30 | 55–65 | [ﾋｷﾀﾞｼ (drawer)](#script-drawer) |
| 31 | 54–64 | [ﾚｲｿﾞｳｺ (fridge)](#script-fridge) |
| 35 | 49–59 | [ｺﾝﾛ (stove)](#script-stove) |
| 39 | 45–55 | [ｶﾝｷｾﾝ (vent fan)](#script-vent-fan) |

If the pair isn't in the table, you misread a digit.

## 3. How to type

### Setup

1. In EmuZ-80K, open **Host**, untick **Use DirectInput**, and restart the emulator (`UseDirectInput=0` in mz80k.ini).
   With DirectInput on, the key handler at 0x4591A0 ignores Windows' key codes, and a per-frame loop (0x459C3A) reads raw scancodes through a Japanese 106-key table (mz80k.exe offset 0xBEE88). On a European keyboard that means `<` does nothing, `Ä` types ﾛ, and no key reaches ﾍ at all. With DirectInput off, Windows' key codes go straight to the MZ-80K key map, and there is no keycode.cfg to remap them.
2. Set Windows to the **US** keyboard layout.

**All key names in this walkthrough are the keycaps printed on a German keyboard, pressed while Windows is set to US.** US layout moves the symbol keys and swaps Y and Z. The table below lists every key that doesn't simply type the kana on its own keycap position, with the US character each one sends:

| Press (German keycap) | Windows sends (US) | types |
|---|---|---|
| `Ö` | `;` | ﾛ |
| `´` | `=` | ﾚ |
| `ß` | `-` | ﾎ |
| `-` | `/` | ﾒ |
| `^` | `` ` `` | ﾍ |
| `Ü` | `[` | ｦ |
| `#` | `\` | ﾑ |
| `+` | `]` | ﾟ |
| `Ä` | `'` | ｹ |
| `<` | `<> (ISO key)` | ﾞ |
| `Z` | `Y` | ﾝ |
| `Y` | `Z` | ﾂ |

**Quick test** once you are in kana mode: press `^ Ö ´ Ä # <` without Enter. You should see `ﾍﾛﾚｹﾑﾞ`. Delete them again with the Delete key, which is the MZ's DEL.

### Kana mode

Every command in this game is kana, and the MZ-80K only types kana in **kana mode**. You switch it on with **SHIFT + カナ**; pressing カナ again without SHIFT switches it off. The monitor's key table (IPL ROM 0x0AC9) puts the カナ key at matrix row 6, bit 5, and EmuZ-80K wires that position to Windows' **VK_KANA** key (key map at mz80k.exe offset 0xD4EA8). US and German keyboards don't have that key. If the emulator isn't already in kana mode, run `kana mode.ps1` from this folder while EmuZ-80K is open: it brings the emulator to the front and presses SHIFT + カナ.

In kana mode, each unshifted key types the kana from the ROM's kana table (IPL ROM 0x0B69). Don't hold SHIFT: SHIFT gives graphics characters, even in kana mode.

### Reading the script lines

Each line answers one prompt. Press the keys on the left in order, then **Enter**. The kana on the right is what should appear on screen. Key names are German keycaps, pressed with Windows set to US:

* `<` is the key left of the Y keycap. It types ﾞ.
* `Num2` and `Num5` are on the number pad, with NumLock on. They type the small ｯ and ｮ.
* `F7` types the long-vowel mark ｰ.
* The MZ-80K doesn't store keypresses, so wait for each `?` prompt before typing the next line.

### Stuck in a question?

A wrong answer never costs time, but most questions keep asking until they get a valid answer. Your next lines then land in the wrong prompt. For example, `ﾑｸ ﾊ ｼﾗﾍﾞ ﾗﾚﾏｾﾝ !!` means the search prompt was still waiting.

* Type **ﾃｲｾｲ** (cancel) to get back to `ﾄﾞｳｽﾙ ?` from the turn prompt, the search prompt, and the `ﾅﾆｦ ｱｹﾏｽｶ ?` open prompt (lines 2040, 5025, 3037). The fridge's part prompt accepts it too (3540). In the chest's part prompt it only goes back to the open prompt, so type it twice (3440).
* The part prompts of the wardrobe (ﾀﾝｽ), the closet (ｵｼｲﾚ), the sink (ﾅｶﾞｼ) and the cupboard (ｼｮｯｷﾀﾞﾅ) have **no** cancel (3218, 3355, 3726, 3960). Give any valid part to get out, then `ｼﾒﾙ` to close it again.
* `… ﾊ ｱｹﾚﾏｾﾝ !!` means the open question (`ﾅﾆｦ ｱｹﾏｽｶ ?`) was still waiting, and what you typed isn't something to open. Type the thing, e.g. ﾄﾞｱ, or ﾃｲｾｲ. If `ﾄﾞｱ` just brings `ﾅﾆｦ ｱｹﾏｽｶ ?` back with no message, you're not facing a door that opens (line 3130). The back room's wall 4 is one of those. Type ﾃｲｾｲ and turn.
* `… ﾊ ｼﾗﾍﾞ ﾗﾚﾏｾﾝ !!` means the search was refused. Either the name is misspelled (check `<` for ﾞ and F7 for ｰ), the thing isn't on the wall you face, or it's inside something that isn't open.

```
W E P E    ﾃｲｾｲ   -> cancel the question
```

Only `ｼﾗﾍﾞﾙ` (search) costs anything. A search that finds nothing prints `ｺｺ ﾆﾊ ｱﾘﾏｾﾝ !!` and adds 2 minutes. Each block ends with a search. On `ｱｯﾀ-------!!!` the memo appears and the game is over; otherwise go on to the next block. The worst case is 7 searches, costing 12 minutes.

### Why pasting doesn't work

EmuZ-80K pastes the clipboard as ANSI text, so on a non-Japanese Windows every kana arrives as `?`. It types `?` as SHIFT + `/`, which is the MZ's `╳` graphic. Pasting ASCII stand-ins can't cover every kana either: the emulator's paste table (mz80k.exe offset 0xB5318) can only reach the ﾞ key with SHIFT held, and can't reach the F7 key that types ｰ at all.

## 4. Getting around

If you lose your place, this section gets you anywhere by hand. Each room has four walls, numbered 1 to 4. What you see on each wall is in the house table in section 6.

* **Turn:** `ﾑｸ` then `ﾋﾀﾞﾘ` goes to the next wall number (4 wraps round to 1); `ﾑｸ` then `ﾐｷﾞ` goes to the previous one.
* **Go through a door:** face it, `ｱｹﾙ` then `ﾄﾞｱ`, then `ｱﾙｸ`. The open doorway between the entrance room and the back room needs only `ｱﾙｸ`.
* **You arrive facing the same wall number** you walked through.
* **Something open?** You can't turn or open anything else until you `ｼﾒﾙ` (close) it. That includes a door you opened but didn't walk through.

```
# H        ﾑｸ
V Q < L    ﾋﾀﾞﾘ   -> turn to the next wall
# H        ﾑｸ
N G <      ﾐｷﾞ    -> turn to the previous wall
3 Ä .      ｱｹﾙ
S < 3      ﾄﾞｱ    -> open the door you face
3 . H      ｱﾙｸ    -> walk through
D - .      ｼﾒﾙ    -> close
```

### Room to room

| From | To | Route |
|---|---|---|
| entrance room | back room | face wall 1, ｱﾙｸ → **back room**, facing wall 1 |
| entrance room | bedroom | face wall 1, ｱﾙｸ → **back room**, facing wall 1; turn ﾋﾀﾞﾘ once, ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **bedroom**, facing wall 2 |
| entrance room | wardrobe room | face wall 4, ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **wardrobe room**, facing wall 4 |
| entrance room | kitchen | face wall 2, ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **kitchen**, facing wall 2 |
| back room | entrance room | face wall 3, ｱﾙｸ → **entrance room**, facing wall 3 |
| back room | bedroom | face wall 2, ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **bedroom**, facing wall 2 |
| back room | wardrobe room | face wall 3, ｱﾙｸ → **entrance room**, facing wall 3; turn ﾋﾀﾞﾘ once, ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **wardrobe room**, facing wall 4 |
| back room | kitchen | face wall 3, ｱﾙｸ → **entrance room**, facing wall 3; turn ﾐｷﾞ once, ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **kitchen**, facing wall 2 |
| bedroom | entrance room | face wall 4, ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **back room**, facing wall 4; turn ﾐｷﾞ once, ｱﾙｸ → **entrance room**, facing wall 3 |
| bedroom | back room | face wall 4, ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **back room**, facing wall 4 |
| bedroom | wardrobe room | face wall 4, ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **back room**, facing wall 4; turn ﾐｷﾞ once, ｱﾙｸ → **entrance room**, facing wall 3; turn ﾋﾀﾞﾘ once, ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **wardrobe room**, facing wall 4 |
| bedroom | kitchen | face wall 4, ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **back room**, facing wall 4; turn ﾐｷﾞ once, ｱﾙｸ → **entrance room**, facing wall 3; turn ﾐｷﾞ once, ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **kitchen**, facing wall 2 |
| wardrobe room | entrance room | face wall 2, ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **entrance room**, facing wall 2 |
| wardrobe room | back room | face wall 2, ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **entrance room**, facing wall 2; turn ﾐｷﾞ once, ｱﾙｸ → **back room**, facing wall 1 |
| wardrobe room | bedroom | face wall 2, ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **entrance room**, facing wall 2; turn ﾐｷﾞ once, ｱﾙｸ → **back room**, facing wall 1; turn ﾋﾀﾞﾘ once, ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **bedroom**, facing wall 2 |
| wardrobe room | kitchen | face wall 2, ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **entrance room**, facing wall 2; ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **kitchen**, facing wall 2 |
| kitchen | entrance room | face wall 4, ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **entrance room**, facing wall 4 |
| kitchen | back room | face wall 4, ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **entrance room**, facing wall 4; turn ﾋﾀﾞﾘ once, ｱﾙｸ → **back room**, facing wall 1 |
| kitchen | bedroom | face wall 4, ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **entrance room**, facing wall 4; turn ﾋﾀﾞﾘ once, ｱﾙｸ → **back room**, facing wall 1; turn ﾋﾀﾞﾘ once, ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **bedroom**, facing wall 2 |
| kitchen | wardrobe room | face wall 4, ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **entrance room**, facing wall 4; ｱｹﾙ ﾄﾞｱ, ｱﾙｸ → **wardrobe room**, facing wall 4 |

### Things to search

Type `ｼﾗﾍﾞﾙ` (`D O ^ < .`), then one of these names at `ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?`. The game only accepts a name on the wall where that thing is, and only once any container it's in is open (lines 5100-6320).

| Name | Keys | What | Where |
|---|---|---|---|
| ﾎﾟｽﾀｰ | `ß + R Q F7` | poster | wardrobe room wall 4, bedroom wall 2 |
| ﾌﾄﾝ | `2 S Z` | futon | closet open (wardrobe room wall 4) |
| ﾊｺ | `F B` | box | closet open, or sink door 4 open |
| ﾎﾞｯｸｽ | `ß < Num2 H R` | chest | chest drawer open (bedroom wall 2) |
| ﾍﾞｯﾄﾞ | `^ < Num2 S <` | bed | bedroom wall 2 |
| ﾎﾟｹｯﾄ | `ß + Ä Num2 S` | pocket | wardrobe door open (wardrobe room wall 1) |
| ﾌｸ | `2 H` | clothes | wardrobe door open |
| ﾋｷﾀﾞｼ | `V G Q < D` | drawer | wardrobe drawer open, or sink drawer 5, 6 or 7 open |
| ﾎﾝ | `ß Z` | books | bedroom wall 3 |
| ﾏﾄﾞ | `J S <` | window | any window: entrance room 3, back room 1, bedroom 1, wardrobe room 3, kitchen 1 |
| ｶﾝｷｾﾝ | `T Z G P Z` | vent fan | kitchen wall 1 |
| ｺﾝﾛ | `B Z Ö` | stove | kitchen wall 1 |
| ﾋﾞﾝ | `V < Z` | bottle | sink door 1 or 2 open |
| ﾎｳﾁｮｳ | `ß 4 A Num5 4` | kitchen knife | sink door 3 open |
| ﾊﾞｹﾂ | `F < Ä Y` | bucket | kitchen wall 1 |
| ｺｯﾌﾟ | `B Num2 2 +` | cup | kitchen wall 3, or a cupboard top door open |
| ｲｽ | `E R` | chair | kitchen wall 3 |
| ﾂｸｴ | `Y H 5` | table | kitchen wall 3 |
| ﾚｲｿﾞｳｺ | `´ E C < 4 B` | fridge | fridge door open (kitchen wall 4) |
| ｻﾗ | `X O` | plates | cupboard top door open (kitchen wall 2) |
| ｳｲｽｷｰ | `4 E R G F7` | whisky | cupboard bottom door open |

### Things to open

Type `ｱｹﾙ` (`3 Ä .`), then the name at `ﾅﾆｦ ｱｹﾏｽｶ ?`, then the part when it asks. Only one thing can be open at a time; `ｼﾒﾙ` (`D - .`) closes it.

| Name | Keys | Where | Parts |
|---|---|---|---|
| ﾄﾞｱ | `S < 3` | any door | — |
| ｵｼｲﾚ | `6 D E ´` | closet, wardrobe room wall 4 | ﾋﾀﾞﾘ `V Q < L`, ﾐｷﾞ `N G <` |
| ﾀﾝｽ | `Q Z R` | wardrobe, wardrobe room wall 1 | ﾄﾋﾀﾞﾘ `S V Q < L`, ﾄﾐｷﾞ `S N G <`, ﾋｷﾀﾞｼｳｴ `V G Q < D 4 5`, ﾋｷﾀﾞｼｼﾀ `V G Q < D D Q` |
| ﾎﾞｯｸｽ | `ß < Num2 H R` | chest of drawers, bedroom wall 2 | ｳｴ `4 5`, ｼﾀ `D Q` |
| ﾅｶﾞｼ | `U T < D` | sink, kitchen wall 1 | ｲﾁ `E A`, ﾆ `I`, ｻﾝ `X Z`, ﾖﾝ `9 Z`, ｺﾞ `B <`, ﾛｸ `Ö H`, ｼﾁ `D A` |
| ｼｮｯｷﾀﾞﾅ | `D Num5 Num2 G Q < U` | cupboard, kitchen wall 2 | ﾋﾀﾞﾘｳｴ `V Q < L 4 5`, ﾐｷﾞｳｴ `N G < 4 5`, ﾋﾀﾞﾘｼﾀ `V Q < L D Q`, ﾐｷﾞｼﾀ `N G < D Q` |
| ﾚｲｿﾞｳｺ | `´ E C < 4 B` | fridge, kitchen wall 4 | ｳｴ `4 5`, ｼﾀ `D Q` |

Parts: ﾋﾀﾞﾘ/ﾐｷﾞ = left/right, ｳｴ/ｼﾀ = top/bottom, ﾄ = door, ﾋｷﾀﾞｼ = drawer, ｲﾁ…ｼﾁ = 1…7 (sink doors 1-4, drawers 5-7).

## 5. Scripts

<a id="script-poster"></a>

### ﾎﾟｽﾀｰ (poster)

2 hiding places: the I LOVE MZ-80 poster (1), the SOFT OF MZ poster (2).

**1.** From the entrance room, wall 1, the open doorway: go to the wardrobe room and search the I LOVE MZ-80 poster.
```
# H           ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <         ﾐｷﾞ     -> now facing wall 4, a door
3 Ä .         ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3         ﾄﾞｱ     -> the door opens
3 . H         ｱﾙｸ     -> walk into the wardrobe room: wall 4, the I LOVE MZ-80 poster + closet
D O ^ < .     ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß + R Q F7    ﾎﾟｽﾀｰ   -> SEARCH
```

**2.** From the wardrobe room, wall 4, the I LOVE MZ-80 poster + closet: go to the bedroom and search the SOFT OF MZ poster.
```
# H           ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L       ﾋﾀﾞﾘ    -> now facing wall 1, the wardrobe
# H           ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L       ﾋﾀﾞﾘ    -> now facing wall 2, the door
3 Ä .         ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3         ﾄﾞｱ     -> the door opens
3 . H         ｱﾙｸ     -> walk into the entrance room: wall 2, a door
# H           ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <         ﾐｷﾞ     -> now facing wall 1, the open doorway
3 . H         ｱﾙｸ     -> walk into the back room: wall 1, the window
# H           ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L       ﾋﾀﾞﾘ    -> now facing wall 2, a door
3 Ä .         ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3         ﾄﾞｱ     -> the door opens
3 . H         ｱﾙｸ     -> walk into the bedroom: wall 2, the bed
D O ^ < .     ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß + R Q F7    ﾎﾟｽﾀｰ   -> SEARCH
```

<a id="script-futon"></a>

### ﾌﾄﾝ (futon)

2 hiding places: the futon, left closet (3), the futon, right closet (4).

**1.** From the entrance room, wall 1, the open doorway: go to the wardrobe room and search the futon, left closet.
```
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <        ﾐｷﾞ     -> now facing wall 4, a door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the wardrobe room: wall 4, the I LOVE MZ-80 poster + closet
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
6 D E ´      ｵｼｲﾚ    -> asks ﾄﾞﾁﾗ ｦ ｱｹﾏｽｶ ?
V Q < L      ﾋﾀﾞﾘ    -> it opens
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
2 S Z        ﾌﾄﾝ     -> SEARCH
```

**2.** Still in the wardrobe room (wall 4, the I LOVE MZ-80 poster + closet): search the futon, right closet.
```
D - .        ｼﾒﾙ     -> close it
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
6 D E ´      ｵｼｲﾚ    -> asks ﾄﾞﾁﾗ ｦ ｱｹﾏｽｶ ?
N G <        ﾐｷﾞ     -> it opens
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
2 S Z        ﾌﾄﾝ     -> SEARCH
```

<a id="script-box"></a>

### ﾊｺ (box)

3 hiding places: the box, left closet (5), the box, right closet (6), the box behind sink door 4 (7).

**1.** From the entrance room, wall 1, the open doorway: go to the kitchen and search the box behind sink door 4.
```
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L      ﾋﾀﾞﾘ    -> now facing wall 2, a door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the kitchen: wall 2, the cupboard
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <        ﾐｷﾞ     -> now facing wall 1, the sink
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
U T < D      ﾅｶﾞｼ    -> asks ﾄﾞｺ ｦ ｱｹﾏｽｶ ?
9 Z          ﾖﾝ      -> it opens
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
F B          ﾊｺ      -> SEARCH
```

**2.** From the kitchen, wall 1, the sink: go to the wardrobe room and search the box, left closet.
```
D - .        ｼﾒﾙ     -> close it
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <        ﾐｷﾞ     -> now facing wall 4, the fridge + door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the entrance room: wall 4, a door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the wardrobe room: wall 4, the I LOVE MZ-80 poster + closet
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
6 D E ´      ｵｼｲﾚ    -> asks ﾄﾞﾁﾗ ｦ ｱｹﾏｽｶ ?
V Q < L      ﾋﾀﾞﾘ    -> it opens
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
F B          ﾊｺ      -> SEARCH
```

**3.** Still in the wardrobe room (wall 4, the I LOVE MZ-80 poster + closet): search the box, right closet.
```
D - .        ｼﾒﾙ     -> close it
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
6 D E ´      ｵｼｲﾚ    -> asks ﾄﾞﾁﾗ ｦ ｱｹﾏｽｶ ?
N G <        ﾐｷﾞ     -> it opens
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
F B          ﾊｺ      -> SEARCH
```

<a id="script-chest"></a>

### ﾎﾞｯｸｽ (chest)

2 hiding places: the chest, top drawer (8), the chest, bottom drawer (9).

**1.** From the entrance room, wall 1, the open doorway: go to the bedroom and search the chest, top drawer.
```
3 . H           ｱﾙｸ     -> walk into the back room: wall 1, the window
# H             ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L         ﾋﾀﾞﾘ    -> now facing wall 2, a door
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3           ﾄﾞｱ     -> the door opens
3 . H           ｱﾙｸ     -> walk into the bedroom: wall 2, the bed
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
ß < Num2 H R    ﾎﾞｯｸｽ   -> asks ﾄﾞﾁﾗ ｦ ｱｹﾏｽｶ ?
4 5             ｳｴ      -> it opens
D O ^ < .       ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß < Num2 H R    ﾎﾞｯｸｽ   -> SEARCH
```

**2.** Still in the bedroom (wall 2, the bed): search the chest, bottom drawer.
```
D - .           ｼﾒﾙ     -> close it
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
ß < Num2 H R    ﾎﾞｯｸｽ   -> asks ﾄﾞﾁﾗ ｦ ｱｹﾏｽｶ ?
D Q             ｼﾀ      -> it opens
D O ^ < .       ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß < Num2 H R    ﾎﾞｯｸｽ   -> SEARCH
```

<a id="script-bed"></a>

### ﾍﾞｯﾄﾞ (bed)

1 hiding place: the bed (10).

**1.** From the entrance room, wall 1, the open doorway: go to the bedroom and search the bed.
```
3 . H           ｱﾙｸ     -> walk into the back room: wall 1, the window
# H             ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L         ﾋﾀﾞﾘ    -> now facing wall 2, a door
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3           ﾄﾞｱ     -> the door opens
3 . H           ｱﾙｸ     -> walk into the bedroom: wall 2, the bed
D O ^ < .       ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
^ < Num2 S <    ﾍﾞｯﾄﾞ   -> SEARCH
```

<a id="script-pocket"></a>

### ﾎﾟｹｯﾄ (pocket)

2 hiding places: the pocket, left wardrobe door (11), the pocket, right wardrobe door (12).

**1.** From the entrance room, wall 1, the open doorway: go to the wardrobe room and search the pocket, left wardrobe door.
```
# H             ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <           ﾐｷﾞ     -> now facing wall 4, a door
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3           ﾄﾞｱ     -> the door opens
3 . H           ｱﾙｸ     -> walk into the wardrobe room: wall 4, the I LOVE MZ-80 poster + closet
# H             ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L         ﾋﾀﾞﾘ    -> now facing wall 1, the wardrobe
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
Q Z R           ﾀﾝｽ     -> asks ﾄﾞｺｦ ｱｹﾏｽｶ ?
S V Q < L       ﾄﾋﾀﾞﾘ   -> it opens
D O ^ < .       ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß + Ä Num2 S    ﾎﾟｹｯﾄ   -> SEARCH
```

**2.** Still in the wardrobe room (wall 1, the wardrobe): search the pocket, right wardrobe door.
```
D - .           ｼﾒﾙ     -> close it
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
Q Z R           ﾀﾝｽ     -> asks ﾄﾞｺｦ ｱｹﾏｽｶ ?
S N G <         ﾄﾐｷﾞ    -> it opens
D O ^ < .       ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß + Ä Num2 S    ﾎﾟｹｯﾄ   -> SEARCH
```

<a id="script-clothes"></a>

### ﾌｸ (clothes)

2 hiding places: the clothes, left wardrobe door (13), the clothes, right wardrobe door (14).

**1.** From the entrance room, wall 1, the open doorway: go to the wardrobe room and search the clothes, left wardrobe door.
```
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <        ﾐｷﾞ     -> now facing wall 4, a door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the wardrobe room: wall 4, the I LOVE MZ-80 poster + closet
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L      ﾋﾀﾞﾘ    -> now facing wall 1, the wardrobe
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
Q Z R        ﾀﾝｽ     -> asks ﾄﾞｺｦ ｱｹﾏｽｶ ?
S V Q < L    ﾄﾋﾀﾞﾘ   -> it opens
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
2 H          ﾌｸ      -> SEARCH
```

**2.** Still in the wardrobe room (wall 1, the wardrobe): search the clothes, right wardrobe door.
```
D - .        ｼﾒﾙ     -> close it
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
Q Z R        ﾀﾝｽ     -> asks ﾄﾞｺｦ ｱｹﾏｽｶ ?
S N G <      ﾄﾐｷﾞ    -> it opens
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
2 H          ﾌｸ      -> SEARCH
```

<a id="script-drawer"></a>

### ﾋｷﾀﾞｼ (drawer)

5 hiding places: the wardrobe's top drawer (15), the wardrobe's bottom drawer (16), sink drawer 5 (17), sink drawer 6 (18), sink drawer 7 (19).

**1.** From the entrance room, wall 1, the open doorway: go to the kitchen and search sink drawer 5.
```
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L      ﾋﾀﾞﾘ    -> now facing wall 2, a door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the kitchen: wall 2, the cupboard
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <        ﾐｷﾞ     -> now facing wall 1, the sink
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
U T < D      ﾅｶﾞｼ    -> asks ﾄﾞｺ ｦ ｱｹﾏｽｶ ?
B <          ｺﾞ      -> it opens
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
V G Q < D    ﾋｷﾀﾞｼ   -> SEARCH
```

**2.** Still in the kitchen (wall 1, the sink): search sink drawer 6.
```
D - .        ｼﾒﾙ     -> close it
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
U T < D      ﾅｶﾞｼ    -> asks ﾄﾞｺ ｦ ｱｹﾏｽｶ ?
Ö H          ﾛｸ      -> it opens
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
V G Q < D    ﾋｷﾀﾞｼ   -> SEARCH
```

**3.** Still in the kitchen (wall 1, the sink): search sink drawer 7.
```
D - .        ｼﾒﾙ     -> close it
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
U T < D      ﾅｶﾞｼ    -> asks ﾄﾞｺ ｦ ｱｹﾏｽｶ ?
D A          ｼﾁ      -> it opens
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
V G Q < D    ﾋｷﾀﾞｼ   -> SEARCH
```

**4.** From the kitchen, wall 1, the sink: go to the wardrobe room and search the wardrobe's top drawer.
```
D - .            ｼﾒﾙ       -> close it
# H              ﾑｸ        -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <            ﾐｷﾞ       -> now facing wall 4, the fridge + door
3 Ä .            ｱｹﾙ       -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3            ﾄﾞｱ       -> the door opens
3 . H            ｱﾙｸ       -> walk into the entrance room: wall 4, a door
3 Ä .            ｱｹﾙ       -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3            ﾄﾞｱ       -> the door opens
3 . H            ｱﾙｸ       -> walk into the wardrobe room: wall 4, the I LOVE MZ-80 poster + closet
# H              ﾑｸ        -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L          ﾋﾀﾞﾘ      -> now facing wall 1, the wardrobe
3 Ä .            ｱｹﾙ       -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
Q Z R            ﾀﾝｽ       -> asks ﾄﾞｺｦ ｱｹﾏｽｶ ?
V G Q < D 4 5    ﾋｷﾀﾞｼｳｴ   -> it opens
D O ^ < .        ｼﾗﾍﾞﾙ     -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
V G Q < D        ﾋｷﾀﾞｼ     -> SEARCH
```

**5.** Still in the wardrobe room (wall 1, the wardrobe): search the wardrobe's bottom drawer.
```
D - .            ｼﾒﾙ       -> close it
3 Ä .            ｱｹﾙ       -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
Q Z R            ﾀﾝｽ       -> asks ﾄﾞｺｦ ｱｹﾏｽｶ ?
V G Q < D D Q    ﾋｷﾀﾞｼｼﾀ   -> it opens
D O ^ < .        ｼﾗﾍﾞﾙ     -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
V G Q < D        ﾋｷﾀﾞｼ     -> SEARCH
```

<a id="script-books"></a>

### ﾎﾝ (books)

1 hiding place: the bookshelf (20).

**1.** From the entrance room, wall 1, the open doorway: go to the bedroom and search the bookshelf.
```
3 . H        ｱﾙｸ     -> walk into the back room: wall 1, the window
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L      ﾋﾀﾞﾘ    -> now facing wall 2, a door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the bedroom: wall 2, the bed
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L      ﾋﾀﾞﾘ    -> now facing wall 3, the bookshelf
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß Z          ﾎﾝ      -> SEARCH
```

<a id="script-window"></a>

### ﾏﾄﾞ (window)

5 hiding places: the wardrobe-room window (21), the back-room window (22), the entrance-room window (23), the kitchen window (24), the bedroom window (25).

**1.** From the entrance room, wall 1, the open doorway: go to the back room and search the back-room window.
```
3 . H        ｱﾙｸ     -> walk into the back room: wall 1, the window
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
J S <        ﾏﾄﾞ     -> SEARCH
```

**2.** From the back room, wall 1, the window: go to the bedroom and search the bedroom window.
```
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L      ﾋﾀﾞﾘ    -> now facing wall 2, a door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the bedroom: wall 2, the bed
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <        ﾐｷﾞ     -> now facing wall 1, the window
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
J S <        ﾏﾄﾞ     -> SEARCH
```

**3.** From the bedroom, wall 1, the window: go to the entrance room and search the entrance-room window.
```
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <        ﾐｷﾞ     -> now facing wall 4, the door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the back room: wall 4, a locked door
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <        ﾐｷﾞ     -> now facing wall 3, the open doorway
3 . H        ｱﾙｸ     -> walk into the entrance room: wall 3, the window
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
J S <        ﾏﾄﾞ     -> SEARCH
```

**4.** From the entrance room, wall 3, the window: go to the wardrobe room and search the wardrobe-room window.
```
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L      ﾋﾀﾞﾘ    -> now facing wall 4, a door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the wardrobe room: wall 4, the I LOVE MZ-80 poster + closet
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <        ﾐｷﾞ     -> now facing wall 3, the window
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
J S <        ﾏﾄﾞ     -> SEARCH
```

**5.** From the wardrobe room, wall 3, the window: go to the kitchen and search the kitchen window.
```
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <        ﾐｷﾞ     -> now facing wall 2, the door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the entrance room: wall 2, a door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the kitchen: wall 2, the cupboard
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <        ﾐｷﾞ     -> now facing wall 1, the sink
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
J S <        ﾏﾄﾞ     -> SEARCH
```

<a id="script-vent-fan"></a>

### ｶﾝｷｾﾝ (vent fan)

1 hiding place: the vent fan (26).

**1.** From the entrance room, wall 1, the open doorway: go to the kitchen and search the vent fan.
```
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L      ﾋﾀﾞﾘ    -> now facing wall 2, a door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the kitchen: wall 2, the cupboard
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <        ﾐｷﾞ     -> now facing wall 1, the sink
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
T Z G P Z    ｶﾝｷｾﾝ   -> SEARCH
```

<a id="script-stove"></a>

### ｺﾝﾛ (stove)

1 hiding place: the stove (27).

**1.** From the entrance room, wall 1, the open doorway: go to the kitchen and search the stove.
```
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L      ﾋﾀﾞﾘ    -> now facing wall 2, a door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the kitchen: wall 2, the cupboard
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <        ﾐｷﾞ     -> now facing wall 1, the sink
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
B Z Ö        ｺﾝﾛ     -> SEARCH
```

<a id="script-bottle"></a>

### ﾋﾞﾝ (bottle)

2 hiding places: the bottle behind sink door 1 (28), the bottle behind sink door 2 (29).

**1.** From the entrance room, wall 1, the open doorway: go to the kitchen and search the bottle behind sink door 1.
```
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L      ﾋﾀﾞﾘ    -> now facing wall 2, a door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the kitchen: wall 2, the cupboard
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <        ﾐｷﾞ     -> now facing wall 1, the sink
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
U T < D      ﾅｶﾞｼ    -> asks ﾄﾞｺ ｦ ｱｹﾏｽｶ ?
E A          ｲﾁ      -> it opens
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
V < Z        ﾋﾞﾝ     -> SEARCH
```

**2.** Still in the kitchen (wall 1, the sink): search the bottle behind sink door 2.
```
D - .        ｼﾒﾙ     -> close it
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
U T < D      ﾅｶﾞｼ    -> asks ﾄﾞｺ ｦ ｱｹﾏｽｶ ?
I            ﾆ       -> it opens
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
V < Z        ﾋﾞﾝ     -> SEARCH
```

<a id="script-kitchen-knife"></a>

### ﾎｳﾁｮｳ (kitchen knife)

1 hiding place: the knife behind sink door 3 (30).

**1.** From the entrance room, wall 1, the open doorway: go to the kitchen and search the knife behind sink door 3.
```
# H             ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L         ﾋﾀﾞﾘ    -> now facing wall 2, a door
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3           ﾄﾞｱ     -> the door opens
3 . H           ｱﾙｸ     -> walk into the kitchen: wall 2, the cupboard
# H             ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <           ﾐｷﾞ     -> now facing wall 1, the sink
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
U T < D         ﾅｶﾞｼ    -> asks ﾄﾞｺ ｦ ｱｹﾏｽｶ ?
X Z             ｻﾝ      -> it opens
D O ^ < .       ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß 4 A Num5 4    ﾎｳﾁｮｳ   -> SEARCH
```

<a id="script-bucket"></a>

### ﾊﾞｹﾂ (bucket)

1 hiding place: the bucket (31).

**1.** From the entrance room, wall 1, the open doorway: go to the kitchen and search the bucket.
```
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L      ﾋﾀﾞﾘ    -> now facing wall 2, a door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the kitchen: wall 2, the cupboard
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <        ﾐｷﾞ     -> now facing wall 1, the sink
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
F < Ä Y      ﾊﾞｹﾂ    -> SEARCH
```

<a id="script-cup"></a>

### ｺｯﾌﾟ (cup)

3 hiding places: the cup on the table (32), the cup, cupboard top left (33), the cup, cupboard top right (34).

**1.** From the entrance room, wall 1, the open doorway: go to the kitchen and search the cup, cupboard top left.
```
# H                    ﾑｸ        -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L                ﾋﾀﾞﾘ      -> now facing wall 2, a door
3 Ä .                  ｱｹﾙ       -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3                  ﾄﾞｱ       -> the door opens
3 . H                  ｱﾙｸ       -> walk into the kitchen: wall 2, the cupboard
3 Ä .                  ｱｹﾙ       -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
D Num5 Num2 G Q < U    ｼｮｯｷﾀﾞﾅ   -> asks ﾄﾞｺ ｦ ｱｹﾏｽｶ ?
V Q < L 4 5            ﾋﾀﾞﾘｳｴ    -> it opens
D O ^ < .              ｼﾗﾍﾞﾙ     -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
B Num2 2 +             ｺｯﾌﾟ      -> SEARCH
```

**2.** Still in the kitchen (wall 2, the cupboard): search the cup, cupboard top right.
```
D - .                  ｼﾒﾙ       -> close it
3 Ä .                  ｱｹﾙ       -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
D Num5 Num2 G Q < U    ｼｮｯｷﾀﾞﾅ   -> asks ﾄﾞｺ ｦ ｱｹﾏｽｶ ?
N G < 4 5              ﾐｷﾞｳｴ     -> it opens
D O ^ < .              ｼﾗﾍﾞﾙ     -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
B Num2 2 +             ｺｯﾌﾟ      -> SEARCH
```

**3.** Still in the kitchen (wall 2, the cupboard): search the cup on the table.
```
D - .         ｼﾒﾙ     -> close it
# H           ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L       ﾋﾀﾞﾘ    -> now facing wall 3, the table
D O ^ < .     ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
B Num2 2 +    ｺｯﾌﾟ    -> SEARCH
```

<a id="script-table"></a>

### ﾂｸｴ (table)

1 hiding place: the table (36).

**1.** From the entrance room, wall 1, the open doorway: go to the kitchen and search the table.
```
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L      ﾋﾀﾞﾘ    -> now facing wall 2, a door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the kitchen: wall 2, the cupboard
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L      ﾋﾀﾞﾘ    -> now facing wall 3, the table
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
Y H 5        ﾂｸｴ     -> SEARCH
```

<a id="script-fridge"></a>

### ﾚｲｿﾞｳｺ (fridge)

2 hiding places: the fridge, top (37), the fridge, bottom (38).

**1.** From the entrance room, wall 1, the open doorway: go to the kitchen and search the fridge, top.
```
# H            ﾑｸ       -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L        ﾋﾀﾞﾘ     -> now facing wall 2, a door
3 Ä .          ｱｹﾙ      -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3          ﾄﾞｱ      -> the door opens
3 . H          ｱﾙｸ      -> walk into the kitchen: wall 2, the cupboard
# H            ﾑｸ       -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L        ﾋﾀﾞﾘ     -> now facing wall 3, the table
# H            ﾑｸ       -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L        ﾋﾀﾞﾘ     -> now facing wall 4, the fridge + door
3 Ä .          ｱｹﾙ      -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
´ E C < 4 B    ﾚｲｿﾞｳｺ   -> asks ﾄﾞﾁﾗ ｦ ｱｹﾏｽｶ ?
4 5            ｳｴ       -> it opens
D O ^ < .      ｼﾗﾍﾞﾙ    -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
´ E C < 4 B    ﾚｲｿﾞｳｺ   -> SEARCH
```

**2.** Still in the kitchen (wall 4, the fridge + door): search the fridge, bottom.
```
D - .          ｼﾒﾙ      -> close it
3 Ä .          ｱｹﾙ      -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
´ E C < 4 B    ﾚｲｿﾞｳｺ   -> asks ﾄﾞﾁﾗ ｦ ｱｹﾏｽｶ ?
D Q            ｼﾀ       -> it opens
D O ^ < .      ｼﾗﾍﾞﾙ    -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
´ E C < 4 B    ﾚｲｿﾞｳｺ   -> SEARCH
```

<a id="script-plates"></a>

### ｻﾗ (plates)

2 hiding places: the plates, cupboard top left (39), the plates, cupboard top right (40).

**1.** From the entrance room, wall 1, the open doorway: go to the kitchen and search the plates, cupboard top left.
```
# H                    ﾑｸ        -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L                ﾋﾀﾞﾘ      -> now facing wall 2, a door
3 Ä .                  ｱｹﾙ       -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3                  ﾄﾞｱ       -> the door opens
3 . H                  ｱﾙｸ       -> walk into the kitchen: wall 2, the cupboard
3 Ä .                  ｱｹﾙ       -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
D Num5 Num2 G Q < U    ｼｮｯｷﾀﾞﾅ   -> asks ﾄﾞｺ ｦ ｱｹﾏｽｶ ?
V Q < L 4 5            ﾋﾀﾞﾘｳｴ    -> it opens
D O ^ < .              ｼﾗﾍﾞﾙ     -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
X O                    ｻﾗ        -> SEARCH
```

**2.** Still in the kitchen (wall 2, the cupboard): search the plates, cupboard top right.
```
D - .                  ｼﾒﾙ       -> close it
3 Ä .                  ｱｹﾙ       -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
D Num5 Num2 G Q < U    ｼｮｯｷﾀﾞﾅ   -> asks ﾄﾞｺ ｦ ｱｹﾏｽｶ ?
N G < 4 5              ﾐｷﾞｳｴ     -> it opens
D O ^ < .              ｼﾗﾍﾞﾙ     -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
X O                    ｻﾗ        -> SEARCH
```

<a id="script-whisky"></a>

### ｳｲｽｷｰ (whisky)

2 hiding places: the whisky, cupboard bottom left (41), the whisky, cupboard bottom right (42).

**1.** From the entrance room, wall 1, the open doorway: go to the kitchen and search the whisky, cupboard bottom left.
```
# H                    ﾑｸ        -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L                ﾋﾀﾞﾘ      -> now facing wall 2, a door
3 Ä .                  ｱｹﾙ       -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3                  ﾄﾞｱ       -> the door opens
3 . H                  ｱﾙｸ       -> walk into the kitchen: wall 2, the cupboard
3 Ä .                  ｱｹﾙ       -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
D Num5 Num2 G Q < U    ｼｮｯｷﾀﾞﾅ   -> asks ﾄﾞｺ ｦ ｱｹﾏｽｶ ?
V Q < L D Q            ﾋﾀﾞﾘｼﾀ    -> it opens
D O ^ < .              ｼﾗﾍﾞﾙ     -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
4 E R G F7             ｳｲｽｷｰ     -> SEARCH
```

**2.** Still in the kitchen (wall 2, the cupboard): search the whisky, cupboard bottom right.
```
D - .                  ｼﾒﾙ       -> close it
3 Ä .                  ｱｹﾙ       -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
D Num5 Num2 G Q < U    ｼｮｯｷﾀﾞﾅ   -> asks ﾄﾞｺ ｦ ｱｹﾏｽｶ ?
N G < D Q              ﾐｷﾞｼﾀ     -> it opens
D O ^ < .              ｼﾗﾍﾞﾙ     -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
4 E R G F7             ｳｲｽｷｰ     -> SEARCH
```

<a id="script-poster-pocket"></a>

### ﾎﾟｽﾀｰ (poster) / ﾎﾟｹｯﾄ (pocket)

4 hiding places: the I LOVE MZ-80 poster (1), the SOFT OF MZ poster (2), the pocket, left wardrobe door (11), the pocket, right wardrobe door (12).

**1.** From the entrance room, wall 1, the open doorway: go to the wardrobe room and search the I LOVE MZ-80 poster.
```
# H           ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <         ﾐｷﾞ     -> now facing wall 4, a door
3 Ä .         ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3         ﾄﾞｱ     -> the door opens
3 . H         ｱﾙｸ     -> walk into the wardrobe room: wall 4, the I LOVE MZ-80 poster + closet
D O ^ < .     ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß + R Q F7    ﾎﾟｽﾀｰ   -> SEARCH
```

**2.** Still in the wardrobe room (wall 4, the I LOVE MZ-80 poster + closet): search the pocket, left wardrobe door.
```
# H             ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L         ﾋﾀﾞﾘ    -> now facing wall 1, the wardrobe
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
Q Z R           ﾀﾝｽ     -> asks ﾄﾞｺｦ ｱｹﾏｽｶ ?
S V Q < L       ﾄﾋﾀﾞﾘ   -> it opens
D O ^ < .       ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß + Ä Num2 S    ﾎﾟｹｯﾄ   -> SEARCH
```

**3.** Still in the wardrobe room (wall 1, the wardrobe): search the pocket, right wardrobe door.
```
D - .           ｼﾒﾙ     -> close it
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
Q Z R           ﾀﾝｽ     -> asks ﾄﾞｺｦ ｱｹﾏｽｶ ?
S N G <         ﾄﾐｷﾞ    -> it opens
D O ^ < .       ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß + Ä Num2 S    ﾎﾟｹｯﾄ   -> SEARCH
```

**4.** From the wardrobe room, wall 1, the wardrobe: go to the bedroom and search the SOFT OF MZ poster.
```
D - .         ｼﾒﾙ     -> close it
# H           ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L       ﾋﾀﾞﾘ    -> now facing wall 2, the door
3 Ä .         ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3         ﾄﾞｱ     -> the door opens
3 . H         ｱﾙｸ     -> walk into the entrance room: wall 2, a door
# H           ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <         ﾐｷﾞ     -> now facing wall 1, the open doorway
3 . H         ｱﾙｸ     -> walk into the back room: wall 1, the window
# H           ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L       ﾋﾀﾞﾘ    -> now facing wall 2, a door
3 Ä .         ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3         ﾄﾞｱ     -> the door opens
3 . H         ｱﾙｸ     -> walk into the bedroom: wall 2, the bed
D O ^ < .     ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß + R Q F7    ﾎﾟｽﾀｰ   -> SEARCH
```

<a id="script-chest-bed"></a>

### ﾎﾞｯｸｽ (chest) / ﾍﾞｯﾄﾞ (bed)

3 hiding places: the chest, top drawer (8), the chest, bottom drawer (9), the bed (10).

**1.** From the entrance room, wall 1, the open doorway: go to the bedroom and search the chest, top drawer.
```
3 . H           ｱﾙｸ     -> walk into the back room: wall 1, the window
# H             ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L         ﾋﾀﾞﾘ    -> now facing wall 2, a door
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3           ﾄﾞｱ     -> the door opens
3 . H           ｱﾙｸ     -> walk into the bedroom: wall 2, the bed
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
ß < Num2 H R    ﾎﾞｯｸｽ   -> asks ﾄﾞﾁﾗ ｦ ｱｹﾏｽｶ ?
4 5             ｳｴ      -> it opens
D O ^ < .       ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß < Num2 H R    ﾎﾞｯｸｽ   -> SEARCH
```

**2.** Still in the bedroom (wall 2, the bed): search the bed.
```
D - .           ｼﾒﾙ     -> close it
D O ^ < .       ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
^ < Num2 S <    ﾍﾞｯﾄﾞ   -> SEARCH
```

**3.** Still in the bedroom (wall 2, the bed): search the chest, bottom drawer.
```
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
ß < Num2 H R    ﾎﾞｯｸｽ   -> asks ﾄﾞﾁﾗ ｦ ｱｹﾏｽｶ ?
D Q             ｼﾀ      -> it opens
D O ^ < .       ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß < Num2 H R    ﾎﾞｯｸｽ   -> SEARCH
```

<a id="script-chest-drawer"></a>

### ﾎﾞｯｸｽ (chest) / ﾋｷﾀﾞｼ (drawer)

7 hiding places: the chest, top drawer (8), the chest, bottom drawer (9), the wardrobe's top drawer (15), the wardrobe's bottom drawer (16), sink drawer 5 (17), sink drawer 6 (18), sink drawer 7 (19).

**1.** From the entrance room, wall 1, the open doorway: go to the kitchen and search sink drawer 5.
```
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L      ﾋﾀﾞﾘ    -> now facing wall 2, a door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the kitchen: wall 2, the cupboard
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <        ﾐｷﾞ     -> now facing wall 1, the sink
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
U T < D      ﾅｶﾞｼ    -> asks ﾄﾞｺ ｦ ｱｹﾏｽｶ ?
B <          ｺﾞ      -> it opens
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
V G Q < D    ﾋｷﾀﾞｼ   -> SEARCH
```

**2.** Still in the kitchen (wall 1, the sink): search sink drawer 6.
```
D - .        ｼﾒﾙ     -> close it
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
U T < D      ﾅｶﾞｼ    -> asks ﾄﾞｺ ｦ ｱｹﾏｽｶ ?
Ö H          ﾛｸ      -> it opens
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
V G Q < D    ﾋｷﾀﾞｼ   -> SEARCH
```

**3.** Still in the kitchen (wall 1, the sink): search sink drawer 7.
```
D - .        ｼﾒﾙ     -> close it
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
U T < D      ﾅｶﾞｼ    -> asks ﾄﾞｺ ｦ ｱｹﾏｽｶ ?
D A          ｼﾁ      -> it opens
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
V G Q < D    ﾋｷﾀﾞｼ   -> SEARCH
```

**4.** From the kitchen, wall 1, the sink: go to the wardrobe room and search the wardrobe's top drawer.
```
D - .            ｼﾒﾙ       -> close it
# H              ﾑｸ        -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <            ﾐｷﾞ       -> now facing wall 4, the fridge + door
3 Ä .            ｱｹﾙ       -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3            ﾄﾞｱ       -> the door opens
3 . H            ｱﾙｸ       -> walk into the entrance room: wall 4, a door
3 Ä .            ｱｹﾙ       -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3            ﾄﾞｱ       -> the door opens
3 . H            ｱﾙｸ       -> walk into the wardrobe room: wall 4, the I LOVE MZ-80 poster + closet
# H              ﾑｸ        -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L          ﾋﾀﾞﾘ      -> now facing wall 1, the wardrobe
3 Ä .            ｱｹﾙ       -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
Q Z R            ﾀﾝｽ       -> asks ﾄﾞｺｦ ｱｹﾏｽｶ ?
V G Q < D 4 5    ﾋｷﾀﾞｼｳｴ   -> it opens
D O ^ < .        ｼﾗﾍﾞﾙ     -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
V G Q < D        ﾋｷﾀﾞｼ     -> SEARCH
```

**5.** Still in the wardrobe room (wall 1, the wardrobe): search the wardrobe's bottom drawer.
```
D - .            ｼﾒﾙ       -> close it
3 Ä .            ｱｹﾙ       -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
Q Z R            ﾀﾝｽ       -> asks ﾄﾞｺｦ ｱｹﾏｽｶ ?
V G Q < D D Q    ﾋｷﾀﾞｼｼﾀ   -> it opens
D O ^ < .        ｼﾗﾍﾞﾙ     -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
V G Q < D        ﾋｷﾀﾞｼ     -> SEARCH
```

**6.** From the wardrobe room, wall 1, the wardrobe: go to the bedroom and search the chest, top drawer.
```
D - .           ｼﾒﾙ     -> close it
# H             ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L         ﾋﾀﾞﾘ    -> now facing wall 2, the door
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3           ﾄﾞｱ     -> the door opens
3 . H           ｱﾙｸ     -> walk into the entrance room: wall 2, a door
# H             ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <           ﾐｷﾞ     -> now facing wall 1, the open doorway
3 . H           ｱﾙｸ     -> walk into the back room: wall 1, the window
# H             ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L         ﾋﾀﾞﾘ    -> now facing wall 2, a door
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3           ﾄﾞｱ     -> the door opens
3 . H           ｱﾙｸ     -> walk into the bedroom: wall 2, the bed
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
ß < Num2 H R    ﾎﾞｯｸｽ   -> asks ﾄﾞﾁﾗ ｦ ｱｹﾏｽｶ ?
4 5             ｳｴ      -> it opens
D O ^ < .       ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß < Num2 H R    ﾎﾞｯｸｽ   -> SEARCH
```

**7.** Still in the bedroom (wall 2, the bed): search the chest, bottom drawer.
```
D - .           ｼﾒﾙ     -> close it
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
ß < Num2 H R    ﾎﾞｯｸｽ   -> asks ﾄﾞﾁﾗ ｦ ｱｹﾏｽｶ ?
D Q             ｼﾀ      -> it opens
D O ^ < .       ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß < Num2 H R    ﾎﾞｯｸｽ   -> SEARCH
```

<a id="script-pocket-table"></a>

### ﾎﾟｹｯﾄ (pocket) / ﾂｸｴ (table)

3 hiding places: the pocket, left wardrobe door (11), the pocket, right wardrobe door (12), the table (36).

**1.** From the entrance room, wall 1, the open doorway: go to the kitchen and search the table.
```
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L      ﾋﾀﾞﾘ    -> now facing wall 2, a door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the kitchen: wall 2, the cupboard
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L      ﾋﾀﾞﾘ    -> now facing wall 3, the table
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
Y H 5        ﾂｸｴ     -> SEARCH
```

**2.** From the kitchen, wall 3, the table: go to the wardrobe room and search the pocket, left wardrobe door.
```
# H             ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L         ﾋﾀﾞﾘ    -> now facing wall 4, the fridge + door
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3           ﾄﾞｱ     -> the door opens
3 . H           ｱﾙｸ     -> walk into the entrance room: wall 4, a door
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3           ﾄﾞｱ     -> the door opens
3 . H           ｱﾙｸ     -> walk into the wardrobe room: wall 4, the I LOVE MZ-80 poster + closet
# H             ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L         ﾋﾀﾞﾘ    -> now facing wall 1, the wardrobe
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
Q Z R           ﾀﾝｽ     -> asks ﾄﾞｺｦ ｱｹﾏｽｶ ?
S V Q < L       ﾄﾋﾀﾞﾘ   -> it opens
D O ^ < .       ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß + Ä Num2 S    ﾎﾟｹｯﾄ   -> SEARCH
```

**3.** Still in the wardrobe room (wall 1, the wardrobe): search the pocket, right wardrobe door.
```
D - .           ｼﾒﾙ     -> close it
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
Q Z R           ﾀﾝｽ     -> asks ﾄﾞｺｦ ｱｹﾏｽｶ ?
S N G <         ﾄﾐｷﾞ    -> it opens
D O ^ < .       ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß + Ä Num2 S    ﾎﾟｹｯﾄ   -> SEARCH
```

<a id="script-pocket-vent-fan"></a>

### ﾎﾟｹｯﾄ (pocket) / ｶﾝｷｾﾝ (vent fan)

3 hiding places: the pocket, left wardrobe door (11), the pocket, right wardrobe door (12), the vent fan (26).

**1.** From the entrance room, wall 1, the open doorway: go to the kitchen and search the vent fan.
```
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L      ﾋﾀﾞﾘ    -> now facing wall 2, a door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the kitchen: wall 2, the cupboard
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <        ﾐｷﾞ     -> now facing wall 1, the sink
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
T Z G P Z    ｶﾝｷｾﾝ   -> SEARCH
```

**2.** From the kitchen, wall 1, the sink: go to the wardrobe room and search the pocket, left wardrobe door.
```
# H             ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <           ﾐｷﾞ     -> now facing wall 4, the fridge + door
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3           ﾄﾞｱ     -> the door opens
3 . H           ｱﾙｸ     -> walk into the entrance room: wall 4, a door
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3           ﾄﾞｱ     -> the door opens
3 . H           ｱﾙｸ     -> walk into the wardrobe room: wall 4, the I LOVE MZ-80 poster + closet
# H             ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L         ﾋﾀﾞﾘ    -> now facing wall 1, the wardrobe
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
Q Z R           ﾀﾝｽ     -> asks ﾄﾞｺｦ ｱｹﾏｽｶ ?
S V Q < L       ﾄﾋﾀﾞﾘ   -> it opens
D O ^ < .       ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß + Ä Num2 S    ﾎﾟｹｯﾄ   -> SEARCH
```

**3.** Still in the wardrobe room (wall 1, the wardrobe): search the pocket, right wardrobe door.
```
D - .           ｼﾒﾙ     -> close it
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
Q Z R           ﾀﾝｽ     -> asks ﾄﾞｺｦ ｱｹﾏｽｶ ?
S N G <         ﾄﾐｷﾞ    -> it opens
D O ^ < .       ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß + Ä Num2 S    ﾎﾟｹｯﾄ   -> SEARCH
```

<a id="script-clothes-drawer"></a>

### ﾌｸ (clothes) / ﾋｷﾀﾞｼ (drawer)

7 hiding places: the clothes, left wardrobe door (13), the clothes, right wardrobe door (14), the wardrobe's top drawer (15), the wardrobe's bottom drawer (16), sink drawer 5 (17), sink drawer 6 (18), sink drawer 7 (19).

**1.** From the entrance room, wall 1, the open doorway: go to the wardrobe room and search the clothes, left wardrobe door.
```
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <        ﾐｷﾞ     -> now facing wall 4, a door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the wardrobe room: wall 4, the I LOVE MZ-80 poster + closet
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L      ﾋﾀﾞﾘ    -> now facing wall 1, the wardrobe
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
Q Z R        ﾀﾝｽ     -> asks ﾄﾞｺｦ ｱｹﾏｽｶ ?
S V Q < L    ﾄﾋﾀﾞﾘ   -> it opens
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
2 H          ﾌｸ      -> SEARCH
```

**2.** Still in the wardrobe room (wall 1, the wardrobe): search the clothes, right wardrobe door.
```
D - .        ｼﾒﾙ     -> close it
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
Q Z R        ﾀﾝｽ     -> asks ﾄﾞｺｦ ｱｹﾏｽｶ ?
S N G <      ﾄﾐｷﾞ    -> it opens
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
2 H          ﾌｸ      -> SEARCH
```

**3.** Still in the wardrobe room (wall 1, the wardrobe): search the wardrobe's top drawer.
```
D - .            ｼﾒﾙ       -> close it
3 Ä .            ｱｹﾙ       -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
Q Z R            ﾀﾝｽ       -> asks ﾄﾞｺｦ ｱｹﾏｽｶ ?
V G Q < D 4 5    ﾋｷﾀﾞｼｳｴ   -> it opens
D O ^ < .        ｼﾗﾍﾞﾙ     -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
V G Q < D        ﾋｷﾀﾞｼ     -> SEARCH
```

**4.** Still in the wardrobe room (wall 1, the wardrobe): search the wardrobe's bottom drawer.
```
D - .            ｼﾒﾙ       -> close it
3 Ä .            ｱｹﾙ       -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
Q Z R            ﾀﾝｽ       -> asks ﾄﾞｺｦ ｱｹﾏｽｶ ?
V G Q < D D Q    ﾋｷﾀﾞｼｼﾀ   -> it opens
D O ^ < .        ｼﾗﾍﾞﾙ     -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
V G Q < D        ﾋｷﾀﾞｼ     -> SEARCH
```

**5.** From the wardrobe room, wall 1, the wardrobe: go to the kitchen and search sink drawer 5.
```
D - .        ｼﾒﾙ     -> close it
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L      ﾋﾀﾞﾘ    -> now facing wall 2, the door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the entrance room: wall 2, a door
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3        ﾄﾞｱ     -> the door opens
3 . H        ｱﾙｸ     -> walk into the kitchen: wall 2, the cupboard
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <        ﾐｷﾞ     -> now facing wall 1, the sink
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
U T < D      ﾅｶﾞｼ    -> asks ﾄﾞｺ ｦ ｱｹﾏｽｶ ?
B <          ｺﾞ      -> it opens
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
V G Q < D    ﾋｷﾀﾞｼ   -> SEARCH
```

**6.** Still in the kitchen (wall 1, the sink): search sink drawer 6.
```
D - .        ｼﾒﾙ     -> close it
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
U T < D      ﾅｶﾞｼ    -> asks ﾄﾞｺ ｦ ｱｹﾏｽｶ ?
Ö H          ﾛｸ      -> it opens
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
V G Q < D    ﾋｷﾀﾞｼ   -> SEARCH
```

**7.** Still in the kitchen (wall 1, the sink): search sink drawer 7.
```
D - .        ｼﾒﾙ     -> close it
3 Ä .        ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
U T < D      ﾅｶﾞｼ    -> asks ﾄﾞｺ ｦ ｱｹﾏｽｶ ?
D A          ｼﾁ      -> it opens
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
V G Q < D    ﾋｷﾀﾞｼ   -> SEARCH
```

<a id="script-chair-whisky"></a>

### ｲｽ (chair) / ｳｲｽｷｰ (whisky)

3 hiding places: the chair (35), the whisky, cupboard bottom left (41), the whisky, cupboard bottom right (42).

**1.** From the entrance room, wall 1, the open doorway: go to the kitchen and search the whisky, cupboard bottom left.
```
# H                    ﾑｸ        -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L                ﾋﾀﾞﾘ      -> now facing wall 2, a door
3 Ä .                  ｱｹﾙ       -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3                  ﾄﾞｱ       -> the door opens
3 . H                  ｱﾙｸ       -> walk into the kitchen: wall 2, the cupboard
3 Ä .                  ｱｹﾙ       -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
D Num5 Num2 G Q < U    ｼｮｯｷﾀﾞﾅ   -> asks ﾄﾞｺ ｦ ｱｹﾏｽｶ ?
V Q < L D Q            ﾋﾀﾞﾘｼﾀ    -> it opens
D O ^ < .              ｼﾗﾍﾞﾙ     -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
4 E R G F7             ｳｲｽｷｰ     -> SEARCH
```

**2.** Still in the kitchen (wall 2, the cupboard): search the whisky, cupboard bottom right.
```
D - .                  ｼﾒﾙ       -> close it
3 Ä .                  ｱｹﾙ       -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
D Num5 Num2 G Q < U    ｼｮｯｷﾀﾞﾅ   -> asks ﾄﾞｺ ｦ ｱｹﾏｽｶ ?
N G < D Q              ﾐｷﾞｼﾀ     -> it opens
D O ^ < .              ｼﾗﾍﾞﾙ     -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
4 E R G F7             ｳｲｽｷｰ     -> SEARCH
```

**3.** Still in the kitchen (wall 2, the cupboard): search the chair.
```
D - .        ｼﾒﾙ     -> close it
# H          ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L      ﾋﾀﾞﾘ    -> now facing wall 3, the table
D O ^ < .    ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
E R          ｲｽ      -> SEARCH
```

<a id="script-poster-bed-pocket"></a>

### ﾎﾟｽﾀｰ (poster) / ﾍﾞｯﾄﾞ (bed) / ﾎﾟｹｯﾄ (pocket)

5 hiding places: the I LOVE MZ-80 poster (1), the SOFT OF MZ poster (2), the bed (10), the pocket, left wardrobe door (11), the pocket, right wardrobe door (12).

**1.** From the entrance room, wall 1, the open doorway: go to the wardrobe room and search the I LOVE MZ-80 poster.
```
# H           ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <         ﾐｷﾞ     -> now facing wall 4, a door
3 Ä .         ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3         ﾄﾞｱ     -> the door opens
3 . H         ｱﾙｸ     -> walk into the wardrobe room: wall 4, the I LOVE MZ-80 poster + closet
D O ^ < .     ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß + R Q F7    ﾎﾟｽﾀｰ   -> SEARCH
```

**2.** Still in the wardrobe room (wall 4, the I LOVE MZ-80 poster + closet): search the pocket, left wardrobe door.
```
# H             ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L         ﾋﾀﾞﾘ    -> now facing wall 1, the wardrobe
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
Q Z R           ﾀﾝｽ     -> asks ﾄﾞｺｦ ｱｹﾏｽｶ ?
S V Q < L       ﾄﾋﾀﾞﾘ   -> it opens
D O ^ < .       ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß + Ä Num2 S    ﾎﾟｹｯﾄ   -> SEARCH
```

**3.** Still in the wardrobe room (wall 1, the wardrobe): search the pocket, right wardrobe door.
```
D - .           ｼﾒﾙ     -> close it
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
Q Z R           ﾀﾝｽ     -> asks ﾄﾞｺｦ ｱｹﾏｽｶ ?
S N G <         ﾄﾐｷﾞ    -> it opens
D O ^ < .       ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß + Ä Num2 S    ﾎﾟｹｯﾄ   -> SEARCH
```

**4.** From the wardrobe room, wall 1, the wardrobe: go to the bedroom and search the bed.
```
D - .           ｼﾒﾙ     -> close it
# H             ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L         ﾋﾀﾞﾘ    -> now facing wall 2, the door
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3           ﾄﾞｱ     -> the door opens
3 . H           ｱﾙｸ     -> walk into the entrance room: wall 2, a door
# H             ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
N G <           ﾐｷﾞ     -> now facing wall 1, the open doorway
3 . H           ｱﾙｸ     -> walk into the back room: wall 1, the window
# H             ﾑｸ      -> asks ﾄﾞﾁﾗｦ ﾑｷﾏｽｶ ?
V Q < L         ﾋﾀﾞﾘ    -> now facing wall 2, a door
3 Ä .           ｱｹﾙ     -> asks ﾅﾆｦ ｱｹﾏｽｶ ?
S < 3           ﾄﾞｱ     -> the door opens
3 . H           ｱﾙｸ     -> walk into the bedroom: wall 2, the bed
D O ^ < .       ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
^ < Num2 S <    ﾍﾞｯﾄﾞ   -> SEARCH
```

**5.** Still in the bedroom (wall 2, the bed): search the SOFT OF MZ poster.
```
D O ^ < .     ｼﾗﾍﾞﾙ   -> asks ﾅﾆ ｦ ｼﾗﾍﾞﾏｽｶ ?
ß + R Q F7    ﾎﾟｽﾀｰ   -> SEARCH
```

## 5. Reference

### Commands (from the in-game list, lines 9200-9213)

| Type (keys) | Then | Does |
|---|---|---|
| ﾑｸ `# H` | ﾋﾀﾞﾘ `V Q < L` / ﾐｷﾞ `N G <` | turn left or right. Only works with nothing open |
| ｱﾙｸ `3 . H` | | walk one step, through the open doorway or through an **open** door |
| ｱｹﾙ `3 Ä .` | ﾄﾞｱ `S < 3`, ﾀﾝｽ `Q Z R`, ｵｼｲﾚ `6 D E ´`, ﾎﾞｯｸｽ `ß < Num2 H R`, ﾚｲｿﾞｳｺ `´ E C < 4 B`, ﾅｶﾞｼ `U T < D`, ｼｮｯｷﾀﾞﾅ `D Num5 Num2 G Q < U`, then which part | open something. Only one thing can be open at a time |
| ｼﾒﾙ `D - .` | | close whatever is open |
| ｼﾗﾍﾞﾙ `D O ^ < .` | object name | search: +2 minutes if the memo isn't there |
| ﾄｹｲ `S Ä E` | | show the clock (minutes:seconds) |
| ﾃｲｾｲ `W E P E` | | at a sub-prompt: cancel |

### The house

Five rooms, each with four walls (BA × HO in the listing, table at line 110).

| Room | Wall 1 | Wall 2 | Wall 3 | Wall 4 |
|---|---|---|---|---|
| entrance room | doorway → back room | door → kitchen | window | door → wardrobe room |
| back room | window | door → bedroom | doorway → entrance room | locked door |
| bedroom | window | bed, SOFT OF MZ poster, chest of drawers (ﾎﾞｯｸｽ) | bookshelf | door → back room |
| wardrobe room | wardrobe (ﾀﾝｽ) | door → entrance room | window | I LOVE MZ-80 poster + closet (ｵｼｲﾚ) |
| kitchen | sink (ﾅｶﾞｼ), stove, vent fan, window, bucket | cupboard (ｼｮｯｷﾀﾞﾅ) | table, chair, cup | door → entrance room, fridge |

Turning left (`ﾑｸ` `ﾋﾀﾞﾘ`) goes from wall 1 to 2 to 3 to 4. Walking through a door keeps the wall number, so after entering the kitchen through wall 2 you face the kitchen's wall 2 (the cupboard).

### Game end

The game ends when you find the memo, or when the clock passes 60:00. The clock is checked after every command and after every wrong search. The END screen prints the clue and its answer. At `TRY AGAIN ? [ Y / N ]`, lines 13010-13020 test for the kana `ﾝ` (again) and `ﾐ` (quit). In kana mode those are the US Y and N keys: press the **Z** keycap to play again, **N** to quit.

Every script was run through a model of the command logic (lines 1000-6560) with the memo placed at each possible hiding place in turn. In every case the script finds the memo, and no command hits a refusal branch. The key names come from the IPL ROM's kana table and the emulator's key map, and haven't been tried in EmuZ-80K yet.
