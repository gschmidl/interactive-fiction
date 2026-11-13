# The Sppy — walkthrough

THE SPPY (ザ・スッパイ), ver 2.2, Momoki Todoroki / YELLOW SOFT, NEC PC-6001 32K.
Goal: you are spy 006, and the top-secret microfilm is somewhere in the "Gemini Building". There
are two identical buildings, left and right. Find the microfilm before dawn and give the password.
The route below takes 40 commands.

## How to type commands

Every command is **two hiragana characters** with the dakuten and handakuten (゛ ゜) left off.
Longer words are cut to their first two kana: ドア → `とあ`, みぎ → `みき`, くるま → `くる`,
パン → `はん`. The keyboard must be in **hiragana** mode: the parser only matches hiragana
codes, so katakana never works.

1. At the first prompt (`どうしよう ?`, what shall I do?) type the verb and press RETURN.
2. If the verb takes an object, a second prompt (`どれ を`, which one?) asks for it.

Movement words (`うえ` up, `した` down, `てる` leave, `みき` right, `ひた` left) need no object.
Anything other than exactly two characters gets `2もじ で にゅうりょく` (enter two characters).

Function keys: F1 `うえ`, F2 `した`, F3 `あけ`, F4 `とあ`, F5 `みる`, F6 `てる`, F7 `ひた`,
F8 `みき`.

**The screen-switch key** (`がめん きりかえ・キー`, as the instructions call it) is the PC-6001's **PAGE**
key. The picture and the text are on different screen pages, which is why the game needs 3
pages. Each room's picture appears first and waits for a key; any key brings up the command
prompt on the text page. While you're at the prompt, PAGE flips back to the picture. In
PC6001V's key map (`pc6001v.ini`), PAGE is on the PC's **Page Up** key.

**Each room understands exactly four commands**, so there is nothing else to find. The
vocabulary is a table in the machine code at `DA3B`: 16 bytes per room, four entries each.
Every command is stored as `RRCA(first byte)`, `CPL(second byte)`, so the list cannot be read
in the listing. The table below is decoded from it. A command the room doesn't know just gives
the prompt again.

**The clock.** The clock face drawn in the corner runs about six hours, to dawn, with a warning
beep every 20 ticks. When it runs out you get `じかん ぎれ  よあけ です` (time's up, it's dawn) and the
program ends. Only a fresh `RUN` resets the clock: **dying sends you back to the street with all
your items gone, but the clock keeps running.**

## The buildings

Death is an explosion and a flash, then a restart on the street.

| Where | The four commands |
|-------|-------------------|
| Street (start) | `のる くる` (get in the car) **or** `あけ しと` (open the seat): **key 1**, only while the car is there, so do it first. `みき` → right 1F. `ひた` → left 1F |
| Right 1F | `うえ` → 2F. `てる` → street. `みる こみ` (look at the rubbish): nothing. **`あけ とあ` = death** |
| Right 2F | **Shootout on entering, every time until the dog is dealt with** (see below). `ける いぬ` (kick the dog) or `やる はん` (give it bread): the dog goes. `うえ` → 3F. `した` → 1F |
| Right 3F | `うえ` → 4F. `した` → 2F. `みる かべ` (look at the wall): nothing. **`あけ とあ` = death** |
| Right 4F | `あけ とあ` → the cage room, but only with key 1; otherwise `かぎがないと、ばくはつする。` (without a key it will explode). `うえ`: `こわれて、 あがれない` (broken, can't go up). `みる しき` (look at the rug): nothing. `した` → 3F |
| Cage room (right 4F) | First time: `おり を ぬけろ !` (get out of the cage). Type `きる` (cut); anything else asks again. Then `うこ かく` (move the furniture): **key 2**. `けす あか` (turn off the light). `てる` → 4F, **only once the light is off**. `きく` (listen): a tune |
| Left 1F | `うえ` → 2F. `てる` → street. `おり ちか` (go down to the basement): **the ending**, once you have done `もつ はち` (else nothing). `あけ とあ` starts a shootout that gains nothing and restarts the game if you lose: skip it |
| Left 2F | `うえ` → 3F. `した` → 1F. `みる かべ`: nothing. **`あけ とあ` = death** |
| Left 3F | `あけ とあ` → the box room, only with key 2 (else the explosion warning). `うえ` → 4F. `した` → 2F. `みる しき`: nothing |
| Box room (left 3F) | `あかり が つきます (サービス)` (the light comes on, free of charge). `あけ はこ` (open the box): **key 3**. `けす あか`, then `てる` → 3F |
| Left 4F | `みる こみ`: a stick shows up in the rubbish. `もつ ほう` (take the stick): you have it. `うえ` → 5F. `した` → 3F |
| Left 5F | `あけ とあ` → the trapdoor room, only with key 3. `うえ`: fireworks (`は な び`) on the roof, then back to 5F. `した` → 4F. `みる しき`: nothing |
| Trapdoor room | Entering from 5F drops you through a trapdoor (see below). `とふ まと` (jump out of the window) → the window room. `けす あか`, then `てる` → 5F. `きく`: a tune |
| Window room | `もつ はち` (pick up the pot): `OK!! このなかに あるが、むりに ひらくと、ばくはつ  する` (it's in here, but forcing it open will make it explode). `けす あか`, then `とふ` or `まと` → back to the trapdoor room |

## The route

| # | Where | Type | Game says / what happens |
|---|-------|------|--------------------------|
| 1 | Street | `のる` `くる` | `かぎ あった !!` (found a key): key 1 |
| 2 | Street | `みき` | right building, 1F |
| 3 | R 1F | `うえ` | 2F, and the shootout starts. Win it |
| 4 | R 2F | `ける` `いぬ` | the dog is gone |
| 5 | R 2F | `うえ` | 3F |
| 6 | R 3F | `うえ` | 4F |
| 7 | R 4F | `あけ` `とあ` | the cage room. `おり を ぬけろ !` |
| 8 | Cage room | `きる` | out of the cage |
| 9 | Cage room | `うこ` `かく` | `かぎ あった !!`: key 2 |
| 10 | Cage room | `けす` `あか` | dark |
| 11 | Cage room | `てる` | 4F |
| 12–14 | R 4F → 1F | `した` ×3 | 1F (no shootout now) |
| 15 | R 1F | `てる` | street |
| 16 | Street | `ひた` | left building, 1F |
| 17–18 | L 1F → 3F | `うえ` ×2 | 3F |
| 19 | L 3F | `あけ` `とあ` | the box room |
| 20 | Box room | `あけ` `はこ` | `かぎ あった !!`: key 3 |
| 21 | Box room | `けす` `あか` | dark |
| 22 | Box room | `てる` | 3F |
| 23 | L 3F | `うえ` | 4F |
| 24 | L 4F | `みる` `こみ` | a stick appears |
| 25 | L 4F | `もつ` `ほう` | you take it |
| 26 | L 4F | `うえ` | 5F |
| 27 | L 5F | `あけ` `とあ` | the trapdoor: press `i`. `ぼう で ささえて たすかった !` (saved by bracing with the stick) |
| 28 | Trapdoor | anything | at `おとしあな を でろ !` (get out of the pit) |
| 29 | Trapdoor room | `とふ` `まと` | the window room |
| 30 | Window room | `もつ` `はち` | `OK!! このなかに あるが...` |
| 31 | Window room | `けす` `あか` | dark |
| 32 | Window room | `とふ` | the trapdoor room (no fall this time) |
| 33 | Trapdoor room | `けす` `あか` | dark |
| 34 | Trapdoor room | `てる` | 5F |
| 35–38 | L 5F → 1F | `した` ×4 | 1F |
| 39 | L 1F | `おり` `ちか` | the ending |
| 40 | Ending | `うめ` | the password |

## The shootout (right 2F)

You are the `I` on the bottom line and the enemy is the `X` at the top. Move with the **←** and
**→** cursor keys, two columns per press. You hit the enemy (`H I T !!`) when a move puts you
directly under it. Every fourth turn it fires a shot straight down its own column: being
under it at that moment is a loss. It drifts randomly. After about 30 turns without a hit you get
`だめ !!  なかま が きた !` (no good, his friends have come) and restart on the street.

You start on column 15 and always move by two, so you can only stand on odd columns. When the X
is on an even column, wait for it to move.

## The trapdoor (left 5F)

Opening the 5F door drops you into a pit. `どうしよう!` (what now!) slides down the screen one line at a time.
**Press lowercase `i` repeatedly** before it reaches the bottom: the program checks for
`CHR$(105)`, so the keyboard must be out of kana mode, and capital `I` does not count. This
only works if you are carrying the stick. Without it, or if you are too slow, you die. Switch
the keyboard to letters before step 27 and back to hiragana afterwards.

## The ending

`4 じげん くうかん` (four-dimensional space), a beach scene captioned `まつわ...!?`, then
`マイクロフィルム が でてきた !!` (the microfilm has appeared) above a strip of coloured blocks, which is the
film. Then the password: `あいことば は 「あい」` (the password is "Ai"). Type **`うめ`**; `あい` does not work. You get two tries; a second wrong
answer restarts the game.

Success: `You are BEST SPY !`, `O K`, and `つぎ の しれい / アメリカ の  N.Y. へ  と べ !!` (next mission:
fly to New York!).

**Nothing in the game or the magazine article gives the password.** It is hidden in line 2150
as `CHR$(&H90+3)+CHR$(&HF0+2)`, which is PC-6001 code `93 F2` = `うめ`. The microfilm picture
contains no letters. Two guesses, neither confirmed:

- The title is read ザ・スッパイ, which sounds like 酸っぱい ("sour"), and 梅 (ume, pickled plum)
  is Japan's classic sour food.
- The caption `まつわ...!?` over a scene with two trees could be 松は ("and the pine…?"), pointing
  at 松竹梅 (pine, bamboo, plum). It is just as likely to be 待つわ ("I'll wait").
