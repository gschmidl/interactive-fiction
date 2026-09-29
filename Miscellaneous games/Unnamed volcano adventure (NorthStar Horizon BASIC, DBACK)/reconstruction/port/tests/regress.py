#!/usr/bin/env python3
"""Scripted checks of volcano.exe.  --seed fixes the time the player takes
to type, which is what the maze's RND(-1) reseeds from (the disk's sector
counter), so each run repeats; seeds 20 apart are the same (a sector passes
every 20 ms).

    regress.py [exe]

  check      HYBASIC tokenises the listing back into DBACK byte for byte
  basic      HYBASIC alone: 1+1 is 2 (the Z80 core's carry after OR), and
             RND(0)'s series at the start; a positive seed repeats, RND(-1)
             reads the disk
  opening    the volcano, the rim, the lava tube, the shaft; a wrong first
             move and the headhunters' feast; the game ends where BASIC says
             READY
  items      the lantern (L) and the gold brick (G); INVENTORY; back to the
             junction
  maze       from MAZE1 the maze always takes three moves (RND(0)'s series
             is the same from every RUN); where it lets you out is RND(-1):
             the pit (the dungeon under construction), the dirt passage, the
             narrow passage, the crevice to the trap door
  trapdoor   the trap door opens while you wait (T): the orcs' feast, or the
             prison; going up the staircase (U)
  fix1       the prison: the capture (17000) is shown - not with --no-fixes
  warps      the author's warps SHAFT and PIT
  length     a move of 73 characters is read; at 74 HYBASIC's 80-column line
             is full: LENGTH ERROR IN LINE 32001, and the port's exit code 1
  crlf       CR LF ends a line once, as CR or LF alone does
  walkthrough  WALKTHROUGH.md's twelve moves reach the prison with every
             seed it names (the feast with 6), and TRAPDOOR, Enter, U does
             whatever the time
"""

import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = os.path.normpath(os.path.join(HERE, '..'))
EXE = os.path.join(PORT, 'volcano.exe')

OPENS = 'THE TRAP DOOR SUDDENLY OPENS WITH A TREMENDOUS\n'
FEAST = 'YOUR HEAD AND PLAY SOCCER WITH IT FOR A WHILE'
DRAG = 'DRAG YOU UP THE SPIRIAL STAIRCASE INTO A DIM,'
EYES = 'AS YOUR EYES SLOWLY BECOME ACCOSTOMED TO THE\n'


def run(lines, *args, eol='\n'):
    p = subprocess.run([EXE] + list(args),
                       input=(eol.join(lines) + eol).encode('latin-1'),
                       capture_output=True, timeout=60)
    if p.returncode != 0 or p.stderr.strip():
        raise AssertionError('exit %d: %s' % (p.returncode, p.stderr))
    return p.stdout.decode('latin-1').replace('\r', '')


def need(out, *texts):
    for t in texts:
        if t not in out:
            raise AssertionError('missing: %r' % t)


def refuse(out, *texts):
    for t in texts:
        if t in out:
            raise AssertionError('should not be there: %r' % t)


def t_check():
    p = subprocess.run([EXE, '--check'], capture_output=True, timeout=60)
    need(p.stdout.decode('latin-1'), 'is DBACK as on the disk (5891 bytes')
    if p.returncode:
        raise AssertionError('exit %d' % p.returncode)


def t_basic():
    out = run(['PRINT 1+1', 'PRINT 0.1+0.2', 'PRINT RND(0),RND(0)',
               'PRINT RND(.5),RND(0)', 'PRINT RND(.5),RND(0)',
               'PRINT RND(-1)'], '--direct', '--seed', '5')
    need(out, 'READY\nPRINT 1+1\n 2\nREADY\n', 'PRINT 0.1+0.2\n .3\n',
         'PRINT RND(0),RND(0)\n .98681641 .78627014\n',
         'PRINT RND(.5),RND(0)\n .74450684 .16030884\nREADY\n'
         'PRINT RND(.5),RND(0)\n .74450684 .16030884\n',
         'PRINT RND(-1)\n 7.4111938E-02\n')


def t_opening():
    out = run(['U', 'D', 'X', 'D', 'U', 'B'], '--seed', '0')
    need(out, '\nYOU ARE ON THE SLOPES OF A VERY OLD, POSSIBLY\n',
         'YOU A CHANCE TO ESCAPE\nMOVE? U\nPOISED ON THE EDGE',
         'MOVE? D\nYOU PLUNGE OVER THE EDGE OF THE PRECIPICE AND FALL\n',
         'MOVE? X\nAN EARTHQUAKE HAS SHUT THE ENTRANCE TO THE LAVA\n',
         'MOVE? D\nYOU FOLLOW THE TUBE DOWN',
         'MOVE? U\nTOO SMALL\nMOVE? B\nNO EXIT.. CAVE IN\nMOVE? ')
    out = run(['X', 'U'], '--seed', '0')
    need(out, 'MOVE? X\nTHE HEADHUNTERS FIRST NEATLY LOP OFF YOUR HEAD',
         'EXPENSE.  TOUGH LUCK FOR YOU.....\n')
    refuse(out, 'READY', 'MOVE? U')
    out = run(['U', 'R'], '--seed', '0')
    need(out, 'MOVE? R\nTHE HEADHUNTERS FIRST')


def t_items():
    out = run(['U', 'D', 'D', 'D', 'INVENTORY', 'R', 'T', 'INVENTORY', 'U',
               'B', 'D'], '--seed', '0')
    need(out, 'SOMETHING, AND DISCOVER A FASCINATING\n'
         'LANTERN-SHAPED OBJECT, COVERED WITH DIRT',
         'MOVE? INVENTORY\n\nYOU HAVE THE FOLLOWING ITEMS:\n\nLANTERN\n\n'
         'MOVE? R\n',
         'YOU NEARLY TRIP OVER A GOLD BRICK\nMOVE? T\n'
         'YOU NOW HAVE THE GOLD BRICK\nMOVE? INVENTORY\n',
         'LANTERN\nGOLD BRICK\n\nMOVE? U\n'
         'YOU ARE BACK AT THE LAVA TUBE/SIDE PASSAGE\nJUNCTION\nMOVE? B\n'
         'YOU ARE STANDING UNDER A\n',
         'MOVE? D\nAHEAD THE DARKNESS CLOSES IN.\nYOU PRESS ON')


def t_maze():
    for seed, text in [('4', "SUDDENLY A SIGN SAYS ... 'DEAD END, DUNGEON\n"
                             'UNDER CONSTRUCTION, PLEASE COME BACK NEXT WEEK'),
                       ('3', 'YOU ARE VIN A NARROW, DIRT FLOORED PASSAGE,\n'
                             'LEADING UP AND CURVING TO THE RIGHT\n'
                             'YOU NEARLY TRIP OVER A GOLD BRICK\nMOVE? '),
                       ('0', 'YOU ARE IN A NARROW, SMOOTH-WALLED PASSAGE\n'
                             'LWEADING UP.  A FAINT LIGHT CAN BE SEEN AHEAD\n'
                             'MOVE? U\nYOU SOON COME TO A SMALL SIDE PASSAGE'),
                       ('2', 'MOVE? D\nYOU MANAGE TO SQUEEZE YOUR WAY INTO A '
                             'TEN INCH\n'),
                       ('22', 'MOVE? D\nYOU MANAGE TO SQUEEZE')]:
        out = run(['MAZE1', 'D', 'D', 'D', 'U'], '--seed', seed)
        need(out, 'MOVE? MAZE1\nYOU ARE IN A BIG TWISTY MAZE OF LITTLE '
             'TUNNELS\nMOVE? D\nYOU ARE IN A TWISTY LITTLE MAZE OF BIG '
             'TUNNELS\nMOVE? D\nYOU ARE IN A BIG TWISTY MAZE OF LITTLE '
             'TUNNELS\nMOVE? D\n', text)


def t_trapdoor():
    out = run(['MAZE1', 'D', 'D', 'D', '', 'T', 'T', 'T', 'T', 'T', 'T'],
              '--seed', '2')
    need(out, 'PERIPHERY OF THE ROOM\n<<<<<<PRESS ENTER TO CONTINUE>>>>>>\n?\n'
         'IN ONE CORNER LIES A LARGE SKELETON',
         'MOVE? T\nWHY DO YOU WANT A SKELETON?\n' + OPENS, FEAST,
         'BEFORE DINING ON YOUR PUNY BODY... TOUGH LUCK.\n')
    refuse(out, EYES, 'READY')
    out = run(['TRAPDOOR', '', 'T'] + ['T'] * 20, '--seed', '0')
    need(out, 'MOVE? TRAPDOOR\nA IRON-BOUND TRAP DOOR IN THE FLOOR',
         'WHY DO YOU WANT A SKELETON?\n' + OPENS, DRAG, EYES,
         '.... SOME HUMANOID, SOME INDETERMINATE.\n')
    refuse(out, FEAST)
    out = run(['MAZE1', 'D', 'D', 'D', '', 'LIFT', 'U'], '--seed', '2')
    need(out, 'MOVE? LIFT\nTOO HEAVY\nMOVE? U\n' + OPENS, DRAG, EYES)


def t_fix1():
    out = run(['TRAPDOOR', '', 'U'], '--seed', '0')
    need(out, 'MOVE? U\n' + OPENS + 'CRASH.  GIANT ORCS SWARM OUT OF THE HOLE, '
         'AND\n' + DRAG, 'TIME BEFORE BEING EATEN\n' + EYES)
    out = run(['TRAPDOOR', '', 'U'], '--seed', '0', '--no-fixes')
    need(out, 'MOVE? U\n' + EYES)
    refuse(out, DRAG, OPENS)


def t_warps():
    out = run(['SHAFT', 'U', 'D'], '--seed', '0')
    need(out, 'MOVE? SHAFT\nYOU ARE STANDING UNDER A\nSMALL SHAFT THAT '
         'APPARENTLY LEADS TO THE SURFACE\nMOVE? U\nTOO SMALL\nMOVE? D\n'
         'AHEAD THE DARKNESS CLOSES IN.\nSOMETHING')
    out = run(['PIT'], '--seed', '0')
    need(out, 'MOVE? PIT\nYOU HAVE TRIPPED IN THE SEMI-DARKNESS OF THE\n',
         'WATER RUSHING, FAR, FAR BELOW YYOU....\n \n',
         'UNDER CONSTRUCTION, PLEASE COME BACK NEXT WEEK\n')
    refuse(out, 'READY')
    out = run(['PIT'], '--seed', '0', '--basic')
    need(out, 'COME BACK NEXT WEEK\nREADY\n')


def t_length():
    out = run(['X' * 73], '--seed', '0')
    need(out, 'MOVE? ' + 'X' * 73 + '\nTHE HEADHUNTERS FIRST')
    p = subprocess.run([EXE, '--seed', '0'], input=b'X' * 74 + b'\n',
                       capture_output=True, timeout=60)
    need(p.stdout.decode('latin-1').replace('\r', ''),
         'MOVE? ' + 'X' * 74 + '\nLENGTH ERROR IN LINE 32001\n')
    if p.returncode != 1:
        raise AssertionError('a BASIC error ends with exit %d, not 1'
                             % p.returncode)


def t_crlf():
    cmds = ['U', 'D', 'D', 'D', 'INVENTORY', 'R', 'T', 'INVENTORY']
    a = run(cmds, '--seed', '0')
    b = run(cmds, '--seed', '0', eol='\r\n')
    c = run(cmds, '--seed', '0', eol='\r')
    if not (a == b == c):
        raise AssertionError('LF, CR LF and CR give different games')


def t_walkthrough():
    # WALKTHROUGH.md: twelve moves, typed in lower case, reach the prison
    # with each seed it names, and the orcs' feast with seed 6
    moves = ['u', 'd', 'd', 'd', 'r', 't', 'd', 'd', 'd', 'd', '', 'u']
    for seed in ('5', '25', '8.25', '10.25', '11.25', '12.25', '13.75',
                 '14.5', '16.25', '16.75', '17.5', '19.75'):
        out = run(moves, '--seed', seed)
        need(out, 'YOU NOW HAVE THE GOLD BRICK',
             'YOU MANAGE TO SQUEEZE YOUR WAY INTO A TEN INCH', OPENS, DRAG,
             EYES)
        refuse(out, FEAST, 'BLACK PIT')
    out = run(moves, '--seed', '6')
    need(out, 'YOU MANAGE TO SQUEEZE', FEAST)
    # TRAPDOOR, Enter, U as the first moves: the prison, whatever the time
    for args in (['--seed', '0'], ['--seed', '7.5'], ['--seed', '13'], []):
        out = run(['trapdoor', '', 'u'], *args)
        need(out, OPENS, DRAG, EYES)
        refuse(out, FEAST)


def main():
    global EXE
    if len(sys.argv) > 1:
        EXE = sys.argv[1]
    bad = 0
    for name, fn in [('check', t_check), ('basic', t_basic),
                     ('opening', t_opening), ('items', t_items),
                     ('maze', t_maze), ('trapdoor', t_trapdoor),
                     ('fix1', t_fix1), ('warps', t_warps),
                     ('length', t_length), ('crlf', t_crlf),
                     ('walkthrough', t_walkthrough)]:
        try:
            fn()
            print('%-9s ok' % name)
        except (AssertionError, subprocess.TimeoutExpired) as e:
            bad += 1
            print('%-9s FAILED: %s' % (name, e))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
