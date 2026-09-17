"""Generate random Skattejakt command scripts from the game's own vocabulary.

usage: fuzzgen.py SEED COUNT OUT [--prefix FILE]

Words that end the session (SLUTT, SPAR, PAUSE, UTSETT) are left out; J is
mixed in now and then so a dead player gets reincarnated.  The plain letter N
answers "no" to the reincarnation question and ends the game, so north is
spelled NORD.  An optional prefix script (e.g. the walk into the cave) runs
first.
"""
import random, sys

MOTION = ('VEI ENTRE OPPOVE NEDOVE SKOG FORTSE SNU DAL TRAPP UT HUS KL\\FT BEKK FJELL '
          'KRYP INN OVERFL M\\RKET TUNNEL LAV KJEMPE UTSIKT OPP NED SJAKT SPREKK KUPPEL '
          'VENSTR H\\YRE HALL HOPP OVER KRYSS \\ST VEST NORD S SYD NORD\\S SYD\\ST SV NV '
          'S\\PPEL HULL VEGG XYZZY PLUGH KLATRE SE ROM HELLER INNGAN HULE ALIBAB MARCOP '
          'LAVERE H\\YERE BAK FORAN STRAND HYLLE KRYSSE ORIENT D U V O').split()
OBJECT = ('N\\KKEL LYKT JERNRI BUR STAV FUGL D\\R PUTE SLANGE SPREKK TAVLEN SKJELL '
          'TROLL DVERG KNIV MAT FLASKE VANN OLJE SPEIL PLANTE \\KS PIRAT DRAGEN JUV BJ\\RN '
          'BESKJE GEYSIR AUTOMA BATTER MOSE TRONE BORD DREGG CELLED PERGAM PLATEN PRINSE '
          'TAU KROK GULL DIAMAN S\\LV JUVELE MYNTER KISTE EGG TREFOR VASE SMARAG PYRAMI '
          'PERLE TEPPE KRYDDE KJEDE SVERD BEGER HARPE LYSEST SCEPTE BRONSE').split()
VERB = ('TA SLIPP SI ]PNE STENG TENN SLUKK VIFT TEM G] DREP HELL SPIS DRIKK GNI KAST '
        'FINN INNHOL FYLL SPRENG POENG KORT LES KNUS VEKK TIDER VASK BUKK FLYTT').split()
SPECIAL = 'FEE FIE FOE FOO FUM HOKUS SESAM HJELP TRE GRAV T]KE INFO SV\\M'.split()


def gen(seed, count):
    r = random.Random(seed)
    out = []
    for _ in range(count):
        x = r.random()
        if x < 0.45:
            out.append(r.choice(MOTION))
        elif x < 0.80:
            out.append('%s %s' % (r.choice(VERB), r.choice(OBJECT)))
        elif x < 0.88:
            out.append(r.choice(OBJECT))
        elif x < 0.95:
            out.append(r.choice(SPECIAL))
        else:
            out.append('J')
    return out


def main():
    seed, count, path = int(sys.argv[1]), int(sys.argv[2]), sys.argv[3]
    prefix = []
    if '--prefix' in sys.argv:
        pf = sys.argv[sys.argv.index('--prefix') + 1]
        prefix = [l.rstrip('\r\n') for l in open(pf, encoding='latin-1') if not l.startswith('#')]
    with open(path, 'w', encoding='latin-1', newline='\n') as f:
        f.write('# fuzzgen.py %d %d%s\n' % (seed, count, ' (with prefix)' if prefix else ''))
        for c in prefix + gen(seed, count):
            f.write(c + '\n')


if __name__ == '__main__':
    main()
