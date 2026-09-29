#!/usr/bin/env python3
"""Hand corrections applied after recase.py.  Each entry replaces the text
(columns 9 on) of a line whose number field and lowercase text match.
Written 2026-09-27 by Claude Opus 5.5 at the direction of Jim Lippard.
"""
import sys

O = [
 # message 1: credits (local text)
 (1, "if you have any problems, suggestions, or questions, please contact",
     "If you have any problems, suggestions, or questions, please contact"),
 (1, "jim lippard (lippard.scouting@pco-multics). you may also",
     "Jim Lippard (Lippard.Scouting@PCO-Multics). You may also"),
 (1, "willie crowther (crowther@parc-maxc), don woods (don@su-ai), and",
     "Willie Crowther (Crowther@PARC-MAXC), Don Woods (Don@SU-AI), and"),
 (1, "gary palter (palter.multics@mit-multics).",
     "Gary Palter (Palter.Multics@MIT-Multics)."),
 # message 142: commands in quotes, as "SUSPEND" etc. in the same message
 (142, 'if you want to end your adventure early, say "quit".',
       'If you want to end your adventure early, say "QUIT".'),
 (142, 'save paper, you may specify "brief", which tells me never to repeat',
       'save paper, you may specify "BRIEF", which tells me never to repeat'),
 (142, 'you ask me to.  to return to all long descriptions, type "full".',
       'you ask me to.  To return to all long descriptions, type "FULL".'),
 (142, 'if you want to see how many turns you have taken, type "turns".',
       'If you want to see how many turns you have taken, type "TURNS".'),
 # dialogue tags after a quotation
 (186, 'he is carrying a large chest.  "shiver me timbers!" he cries, "i\'ve',
       'He is carrying a large chest.  "Shiver me timbers!" he cries, "I\'ve'),
 (240, 'marked "ralph" on his shirt. "you blithering idiot!" he storms.',
       'marked "Ralph" on his shirt. "You blithering idiot!" he storms.'),
 # message 202: the 1980 news (local text)
 (202, '             translated from "spelunker today", may 1980 issue',
       '             Translated from "Spelunker Today", May 1980 issue'),
 (202, '                    *** more new features ***',
       '                    *** More New Features ***'),
 (202, '     the cave\'s most recent features aside from many new',
       '     The cave\'s most recent features aside from many new'),
 (202, 'first one is "fast", which suppresses *all* long',
       'first one is "FAST", which suppresses *all* long'),
 (202, 'descriptions. the second is "full", which prints only long',
       'descriptions. The second is "FULL", which prints only long'),
 (202, 'descriptions. the third is "listen" which just tells what',
       'descriptions. The third is "LISTEN" which just tells what'),
 (202, 'sounds are around you. the fourth is "turns" which tells you',
       'sounds are around you. The fourth is "TURNS" which tells you'),
 (202, 'just modified. "score" no longer asks you if you want to quit.',
       'just modified. "SCORE" no longer asks you if you want to quit.'),
 (202, 'you may obtain information on other games by typing "games".',
       'You may obtain information on other games by typing "GAMES".'),
 (202, 'please contact jjl.sct if you have any problems or suggestions.',
       'Please contact jjl.sct if you have any problems or suggestions.'),
 # message 203: Multics pathnames stay as printed
 (203, 'other adventure-type games on multics:',
       'Other adventure-type games on Multics:'),
 (203, '>udd>micro>demo>wumpus (is wumpus ii)',
       '>udd>micro>demo>wumpus (is Wumpus II)'),
 # the djinn: ordinary sentence case (Platt 1984 has his speech in capitals,
 # which matched only piecemeal)
 (266, 'the flask\'s wax seal breaks away easily, and a cloud of dark smoke',
       'The flask\'s wax seal breaks away easily, and a cloud of dark smoke'),
 (266, 'twelve-foot djinn standing in the pentagram. he pushes experimentally',
       'twelve-foot djinn standing in the pentagram. He pushes experimentally'),
 (266, 'to you. "my thanks, oh mortal," he says in an incredibly deep bass',
       'to you. "My thanks, oh mortal," he says in an incredibly deep bass'),
 (266, 'voice. "it has been three thousand years since solomon sealed me',
       'voice. "It has been three thousand years since Solomon sealed me'),
 (266, 'into that bottle, and i am grateful that you have released me. if',
       'into that bottle, and I am grateful that you have released me. If'),
 (266, 'you will open this pentagram and let me go free, i will give you',
       'you will open this pentagram and let me go free, I will give you'),
 (266, 'some advice that you may one day wish to possess."',
       'some advice that you may one day wish to possess."'),
 (268, 'the flask\'s wax seal crumbles at your touch. a large cloud of black',
       'The flask\'s wax seal crumbles at your touch. A large cloud of black'),
 (268, '"at last!" he says in an earth-shaking voice, "i knew that someday',
       '"At last!" he says in an earth-shaking voice, "I knew that someday'),
 (268, 'someone would release me! i would reward you for this, mortal, but',
       'someone would release me! I would reward you for this, mortal, but'),
 (268, 'it has been three thousand years since i had a solid meal, and i\'m',
       'it has been three thousand years since I had a solid meal, and I\'m'),
 (268, 'not going to stand here chattering when i could be out eating a six-',
       'not going to stand here chattering when I could be out eating a six-'),
 (268, 'inch sirloin steak. farewell." with that, he somewhat rudely explodes',
       'inch sirloin steak. Farewell." With that, he somewhat rudely explodes'),
 (270, 'the djinn stretches gratefully and smiles at you. "again my thanks,"',
       'The djinn stretches gratefully and smiles at you. "Again my thanks,"'),
 (270, 'he says, "i will tell you a piece of ancient lore which i learned from',
       'he says, "I will tell you a piece of ancient lore which I learned from'),
 (270, 'my aunt, an afreet of great knowledge. there is another magic word',
       'my aunt, an afreet of great knowledge. There is another magic word'),
 (270, 'that you might find of use if you should ever find yourself being',
       'that you might find of use if you should ever find yourself being'),
 (270, 'attacked by those pestiferous dwarves. you should only use it as',
       'attacked by those pestiferous dwarves. You should only use it as'),
 (270, 'a last resort, though, since it is a most potent word and is',
       'a last resort, though, since it is a most potent word and is'),
 (270, 'prone to backfire for no obvious reason; also, it should never',
       'prone to backfire for no obvious reason; also, it should never'),
 (270, 'be used near water or any sharp weapon or the results may be most',
       'be used near water or any sharp weapon or the results may be most'),
 (270, 'must be pronounced carefully if it is to have the proper effect.',
       'must be pronounced carefully if it is to have the proper effect.'),
 (270, 'farewell, and good luck!" with that, the djinn-cloud drifts away',
       'Farewell, and good luck!" With that, the djinn-cloud drifts away'),
 (100, 'there is a twelve-foot djinn standing in the pentagram,',
       'There is a twelve-foot djinn standing in the pentagram,'),
 # short description matching its long description
 (148, "you're in sorcerer's lair.", "You're in Sorcerer's Lair."),
 # quoted sign and magic phrase in the sorcerer's lair / ice room
 (157, 'a recently made sign placed near the icicles reads "ice tunnels',
       'A recently made sign placed near the icicles reads "Ice tunnels'),
 (148, 'reads "noside samoht".  strange shadows flit about on the walls, but',
       'reads "NOSIDE SAMOHT".  Strange shadows flit about on the walls, but'),
 # magic messages
 (21, '(e.g. 14, not 14:00 or 2pm).  enter a negative number after last pair.',
      '(e.g. 14, not 14:00 or 2pm).  Enter a negative number after last pair.'),
 (22, 'new hours for colossal cave:', 'New hours for Colossal Cave:'),
]

def main():
    inp, outp = sys.argv[1:3]
    lines = open(inp).read().split('\n')
    used = [0] * len(O)
    for i, l in enumerate(lines):
        num, text = l[:8].strip(), l[8:]
        if text.startswith('>$<'):
            lines[i] = l[:8] + text.lower()
            continue
        for k, (n, old, new) in enumerate(O):
            if num == str(n) and text.lower() == old:
                assert new.lower() == old, (old, new)
                lines[i] = l[:8] + new
                used[k] += 1
    for k, u in enumerate(used):
        if u != 1:
            print('override used %d times: %r' % (u, O[k][1]))
    open(outp, 'w').write('\n'.join(lines))

main()
