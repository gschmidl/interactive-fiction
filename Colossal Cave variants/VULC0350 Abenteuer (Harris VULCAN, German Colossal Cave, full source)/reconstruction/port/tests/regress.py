#!/usr/bin/env python3
"""Regression tests for the ABENTEUER port.

    python tests\\regress.py [-v]

Every game is played in a scratch copy of the port (tests\\transcript.py),
so NEUSPIEL.DAT and saves\\ are never touched, with the clock held by
--date and --time: the cave keeps the site's hours (weekdays 8-18 only
wizards), and a restored game must have waited LATNCY (90) minutes.
At the end, tests\\crosscheck.py (the port's own set-up against the
site's NEUSPIEL) and tests\\win.py (350 of 350) are run too.
-v prints every transcript.
"""
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import crosscheck  # noqa: E402
import state  # noqa: E402
import transcript as T  # noqa: E402
import win  # noqa: E402

VERBOSE = '-v' in sys.argv[1:]
SUNDAY = ['--date', '2026-09-20', '--time', '20:00']
MONDAY = ['--date', '2026-09-21', '--time', '10:00']
failed = []


def report(name, ok, text=''):
    print('%-44s %s' % (name, 'ok' if ok else 'FAILED'))
    if not ok:
        failed.append(name)
    if text and (VERBOSE or not ok):
        print('    ' + text.replace('\n', '\n    '))


def in_order(text, parts):
    """every part in TEXT, one after another; the first one missing"""
    at = 0
    for p in parts:
        i = text.find(p, at)
        if i < 0:
            return p
        at = i + len(p)
    return None


def game(name, commands, args, parts, where=None, code=0):
    text, rc = T.run(commands, args, where)
    miss = in_order(text, parts)
    report(name, miss is None and rc == code,
           text + ('\n[missing: %s]' % miss if miss else '') +
           ('\n[exit %d, not %d]' % (rc, code) if rc != code else ''))
    return text


def run_exe(args):
    r = subprocess.run([os.path.join(PORT, 'abenteuer.exe')] + args,
                       capture_output=True, timeout=30)
    return r.returncode, (r.stdout + r.stderr).decode('latin-1')


# ------------------------------------------------------------ the wizard
def wizard_reply(text):
    """WIZARD's challenge: ten octal digits; the reply is the magic word
    (CODE1, 6-bit fields = CHRSET index - 1) exclusive-or those 30 bits,
    typed as the five characters whose fields they are"""
    digits = [l.strip() for l in text.split('\n')
              if len(l.strip()) == 10 and l.strip().isdigit()][-1]
    magic = state.State(os.path.join(PORT, 'NEUSPIEL.DAT'))['MAGIC']
    word = magic ^ int(digits, 8)
    chars = ''
    for sh in (24, 18, 12, 6, 0):
        f = (word >> sh) & 63
        if f == 63 or (f == 0 and sh and (word & ((1 << sh) - 1))):
            raise ValueError('no reply can be typed for %s' % digits)
        chars += '_' if f == 62 else chr(32 + f)
    return chars.rstrip()


def main():
    sys.stdout.reconfigure(errors='backslashreplace')
    game('opening (NEUSPIEL, turn 1)', [], SUNDAY,
         ['DU BIST AM ENDE DER STRASSE.'])
    game('commands, lower case', ['osten', 'nimm lampe', 'bestand', 'westen'],
         SUNDAY, ['DU BEFINDEST DICH INNERHALB EINES GEBAEUDES.',
                  'DA LIEGT EINE BLANKE KUPFERLAMPE.', '> nimm lampe', 'OK',
                  'DU HAELST ZUR ZEIT FOLGENDES:\nKUPFERLAMPE',
                  '> westen', 'DU BIST AM ENDE DER STRASSE.'])
    # bit 8 is masked off (\xe1 is a) and NULs dropped
    game('commands, upper case, bit 8 and NULs',
         ['OSTEN', 'n\x80i\x00mm l\xe1mpe', 'bestand'],
         SUNDAY, ['DU BEFINDEST DICH INNERHALB EINES GEBAEUDES.', 'OK',
                  'DU HAELST ZUR ZEIT FOLGENDES:\nKUPFERLAMPE'])
    game('unknown word', ['inventar'], SUNDAY, ['DAS VERSTEHE ICH NICHT!'])
    game('score, and not quitting', ['punkte', 'vielleicht', 'nein', 'osten'],
         SUNDAY, ['HAETTEST DU  32 VON MOEGLICHEN 350 PUNKTEN.',
                  'WILLST DU ETWA AUFGEBEN?', 'BITTE BEANTWORTE DIE FRAGE.',
                  'WILLST DU ETWA AUFGEBEN?', 'OK',
                  'DU BEFINDEST DICH INNERHALB EINES GEBAEUDES.'])
    # the word is AUFGA(BE): "aufgeben" is AUFGE, not a word
    game('quitting', ['aufgabe', 'ja'], SUNDAY,
         ['WILLST DU WIRKLICH AUFGEBEN?', '> ja', 'OK',
          'DU ERZIELTEST  32 VON MOEGLICHEN 350 PUNKTEN MIT    2 ZUEGEN.',
          'DU BIST OFFENBAR EIN BLUTIGER ANFAENGER.'])

    # the site's hours
    game('prime time: closed, not a wizard', ['nein'], MONDAY,
         ['TUT MIR SEHR LEID, ABER DIE HOEHLE IST GESCHLOSSEN.',
          'MO  - FR :   0:00 BIS  8:00', '18:00 BIS 24:00',
          'NUR ZAUBERER HABEN ZUR ZEIT ZUTRITT ZUR HOEHLE.',
          'BIST DU EIN ZAUBERER?', 'SEHR SCHOEN.',
          'ICH NEHME AN, SETZT DEIN ABENTEUER SPAETER FORT.'])
    game('prime time: -u opens the cave', ['osten'], MONDAY + ['-u'],
         ['DU BIST AM ENDE DER STRASSE.',
          'DU BEFINDEST DICH INNERHALB EINES GEBAEUDES.'])
    game('weekday 18:00 is open', ['osten'],
         ['--date', '2026-09-21', '--time', '18:00'],
         ['DU BIST AM ENDE DER STRASSE.',
          'DU BEFINDEST DICH INNERHALB EINES GEBAEUDES.'])
    game('Saturday morning is open', ['osten'],
         ['--date', '2026-09-19', '--time', '10:00'],
         ['DU BIST AM ENDE DER STRASSE.',
          'DU BEFINDEST DICH INNERHALB EINES GEBAEUDES.'])

    # SICHR and BRING
    where = T.scratch()
    try:
        def at(hhmm, *extra):
            return ['--date', '2026-09-20', '--time', hhmm] + list(extra)
        game('SICHR', ['osten', 'nimm lampe', 'sichr test', 'ja'], at('20:00'),
             ['ICH KANN DEIN ABENTEUER SICHERN', 'MINDESTENS 90 MINUTEN',
              'IST DAS ANNEHMBAR?', '> ja', 'OK'], where)
        report('SICHR wrote saves\\TEST.SAV',
               os.path.exists(os.path.join(where, 'saves', 'TEST.SAV')))
        game('BRING after 10 minutes', ['bring test'], at('20:10'),
             ['DIESES ABENTEUER WURDE VOR KAUM 10 MINUTEN GESICHERT',
              'SELBST ZAUBERER MUESSEN LAENGER WARTEN!'], where)
        game('BRING after 40 minutes: wizards only', ['bring test', 'nein'],
             at('20:40'),
             ['VOR KAUM 40 MINUTEN GESICHERT',
              'NUR ZAUBERER DUERFEN IHR ABENTEUER SO FRUEH FORTSETZEN.',
              'BIST DU EIN ZAUBERER?',
              'ICH NEHME AN, SETZT DEIN ABENTEUER SPAETER FORT.'], where)
        game('BRING after 91 minutes', ['bring test', 'bestand'], at('21:31'),
             ['> bring test', 'DU BIST IM GEBAEUDE.', '> bestand',
              'KUPFERLAMPE'], where)
        game('BRING at once with -u', ['bring test', 'bestand'],
             at('20:01', '-u'),
             ['> bring test', 'DU BIST IM GEBAEUDE.', 'KUPFERLAMPE'], where)
        game('BRING of no such game: NEUSPIEL', ['bring nichts', 'bestand'],
             at('23:00'),
             ['> bring nichts', 'DU BIST AM ENDE DER STRASSE.',
              'DU TRAEGST NICHTS.'], where)
        game('--no-fixes: BRING not taken at turn 1', ['bring test'],
             at('23:00', '--no-fixes'), ['DAS WORT KENNE ICH NICHT.'], where)
        game('SICHR without a name', ['sichr', 'ja'], at('23:00'),
             ['IST DAS ANNEHMBAR?',
              'TUT MIR LEID, ABER DEINEN FILE KANN ICH WEDER GENERIEREN '
              'NOCH FINDEN.'], where)
    finally:
        shutil.rmtree(where, ignore_errors=True)

    # the wizard
    game('MAGIE MODUS: an impostor', ['magie modus', 'ja', 'xyzzy'], SUNDAY,
         ['BIST DU EIN ZAUBERER?', 'BEWEISE ES! SAG DAS MAGISCHE WORT!',
          'BUH, DU BIST NICHTS ALS EIN SCHARLATAN!'])
    where = T.scratch()
    try:
        game('MAGIE MODUS: maintenance, NEUSPIEL saved',
             ['magie modus', 'ja', 'dwarf', 'nein', wizard_reply, 'nein',
              'nein', 'nein', '', '', '', 'nein', 'bestand'],
             SUNDAY + ['--seed', '1'],
             ['DO YOU KNOW WHAT I THOUGHT IT WAS?',
              'OH DEAR, YOU REALLY *ARE* A WIZARD!',
              'MOECHTEST DU DIE ZEITEN SEHEN?',
              'MOECHTEST DU DIE ZEITEN AENDERN?',
              'DO YOU WISH TO (RE)SCHEDULE THE NEXT HOLIDAY?',
              'LENGTH OF SHORT GAME (NULL TO LEAVE AT  30):',
              'NEW MAGIC WORD (NULL TO LEAVE UNCHANGED):',
              'LATENCY FOR RESTART (NULL TO LEAVE AT  90):',
              'DO YOU WISH TO CHANGE THE MESSAGE OF THE DAY?',
              'THE NEW VERSION HAS BEEN SAVED, THANK YOU...',
              'DU TRAEGST NICHTS.'], where)
        s = state.State(os.path.join(where, 'NEUSPIEL.DAT'))
        report('... the new NEUSPIEL is set up (SETUP 2)', s['SETUP'] == 2)
        game('... and a game from it starts anew', ['nein', 'osten'], SUNDAY,
             ['WILLKOMMEN ZUM ABENTEUER! MOECHTEST DU HINWEISE?',
              'DU STEHST AM ENDE EINER STRASSE VOR EINEM KLEINEN HAUS.',
              'DU BEFINDEST DICH INNERHALB EINES GEBAEUDES.'], where)
    finally:
        shutil.rmtree(where, ignore_errors=True)

    # --fresh
    game('--fresh: set up from ADV.DATA', ['nein', 'nein', 'osten'],
         SUNDAY + ['--fresh'],
         ['INITIALIZING...', 'TABLE SPACE USED:',
          '9671 OF   9800 WORDS OF MESSAGES', '289 OF    300 VOCABULARY WORDS',
          'BIST DU EIN ZAUBERER?', 'SEHR SCHOEN.',
          'INITIALIZATION COMPLETED.',
          'WILLKOMMEN ZUM ABENTEUER! MOECHTEST DU HINWEISE?',
          'DU STEHST AM ENDE EINER STRASSE VOR EINEM KLEINEN HAUS.',
          'DU BEFINDEST DICH INNERHALB EINES GEBAEUDES.'])
    # the 1980 edition has no blank vocabulary entry 0: one word fewer
    game('--fresh=1980: set up from ADV1980.DATA', ['nein', 'nein', 'osten'],
         SUNDAY + ['--fresh=1980'],
         ['INITIALIZING...', '288 OF    300 VOCABULARY WORDS',
          'INITIALIZATION COMPLETED.',
          'DU STEHST AM ENDE EINER STRASSE VOR EINEM KLEINEN HAUS.',
          'DU BEFINDEST DICH INNERHALB EINES GEBAEUDES.'])

    # options
    rc, out = run_exe(['--help'])
    report('--help', rc == 0 and out.startswith('usage: abenteuer'), out)
    for bad in (['--bogus'], ['--seed', 'x'], ['--date', '2026-13-01'],
                ['--time', '25:00'], ['-x'], ['--fresh=1999']):
        rc, out = run_exe(bad)
        report('refused: ' + ' '.join(bad),
               rc == 2 and 'unknown option' in out, out)

    # the whole state, and the end
    report('crosscheck: set-up equals the site\'s NEUSPIEL',
           not crosscheck.main())
    report('win: 350 of 350', win.main() == 0)

    print('regress: %d failed' % len(failed) if failed else 'regress: all ok')
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
