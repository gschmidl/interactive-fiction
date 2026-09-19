"""Random SVHA Adventure command scripts from the game's own vocabulary.

usage: svhafuzz.py SEED COUNT OUT [--prefix FILE]

QUIT is left out.  The plain letter N answers "no" to the reincarnation
question and ends the game, so north is spelled NORTH; YES is mixed in now
and then.  The words are SVHA-DATAFIL's word table.
"""
import random, sys

MOTION = ('IN OUT ENTER EXIT LEAVE PASSAG TUNNEL UPWARD DOWNWARD ABOVE BELOW UPSTREAM '
          'DOWNSTREAM JUMP CLIMB OVER ACROSS CROSS BUILDING HOUSE Y2 XYZZY PLUGH PLOVER '
          'SHORE EXPLORE FURTHER LEDGE APPROACH BEHIND FRONT LEFT RIGHT NORTH NORTHEAST '
          'EAST SOUTHEAST SOUTH SOUTHWEST WEST NORTHWEST UP DOWN NE E SE S SW W NW U D '
          'BARREN BED BEDQUILT BROKEN CAVERN COBBLE CRAWL DARK DEBRIS ROOM ENTRANCE FLOOR '
          'FOREST FORK GIANT GRATE GULLY HALL HILL HOLE ORIENTAL OUTDOORS STEPS PIT '
          'RESERVOIR ROAD SECRET SHELL SLAB STAIRCASE DANCING SURFACE CANYON VALLEY VIEW '
          'DUNGEON DOME BACK RETREAT RETURN').split()
OBJECT = ('KEYS LAMP BOTTLE FOOD CAGE ROD BIRD PILLOW BATTERIES AXE SWORD GRAPHNEL MAGAZINE '
          'CLAM OYSTER BEAR SHARDS WATER OIL KNIFE HAND WINE CAULDRON SOUP PICKAX SHOVEL '
          'KNOT ENVELOPE PILL RING METAL LEATHER SATCHEL GOLD NUGGET DIAMONDS JEWELRY COINS '
          'SILVER TRIDENT EMERALD PYRAMID PEARL RUG VASE SPICES CHAIN EGGS NEST CHANDELIER '
          'SCEPTER CHALICE HARP VESSEL COAT MITHRIL RUBY SAPHIRE CIRCLE CHEST BOX TREASURE '
          'BRACELET GRATE STEPS DOOR FISSURE BRIDGE TABLET PLANT BEANSTALK SHADOW DRAWINGS '
          'CHASM MESSAGE VOLCANO GEYSER MACHINE CARPET MOSS THRONE TABLE CELLDOOR PARCHMENT '
          'ROPE HOOK BOARD MIRROR RUNES BARREL LEAVES STALACTITE ARMOUR TREES ICE TOMB '
          'PORTAL PILLAR LINTEL FIRE LID TAPESTRY DRAPERY ORNAMENT FRIEZE BORDER MIST GATE '
          'COFFIN VAULT ALTAR SHELF WALL STONE ROCK KING WIZARD FAERIE HAG WITCH SERPENT '
          'PIRATE DRAGON SNAKE ELVES ORCS GHOST TROLL PRINCE DWARF').split()
VERB = ('TAKE DROP LOOK EXAMINE INVENTORY GO WALK RUN FOLLOW UNLOCK OPEN LOCK CLOSE POUR '
        'LIGHT EXTINGUISH WAVE SHAKE ATTACK KILL EAT DRINK FILL FEED THROW RUB READ BREAK '
        'WASH BOW FIND BLAST CALM WAKE DIG SAY SING HOURS SWIM CUT PUT LIFT TIE '
        'GIVE KICK REMOVE').split()
SPECIAL = ('HELP INFO FEE FIE FOE FOO FUM SESAME ABRACADABRA SHAZAM HOCUS POCUS SINBAD '
           'ALIBABA NOTHING JUJU').split()


def gen(seed, count):
    r = random.Random(seed)
    out = []
    for _ in range(count):
        x = r.random()
        if x < 0.45:
            out.append(r.choice(MOTION))
        elif x < 0.80:
            out.append('%s %s' % (r.choice(VERB), r.choice(OBJECT)))
        elif x < 0.87:
            out.append(r.choice(OBJECT))
        elif x < 0.94:
            out.append(r.choice(SPECIAL))
        else:
            out.append('YES')
    return out


def main():
    seed, count, path = int(sys.argv[1]), int(sys.argv[2]), sys.argv[3]
    prefix = []
    if '--prefix' in sys.argv:
        pf = sys.argv[sys.argv.index('--prefix') + 1]
        prefix = [l.rstrip('\r\n') for l in open(pf, encoding='latin-1') if not l.startswith('#')]
    with open(path, 'w', encoding='latin-1', newline='\n') as f:
        f.write('# svhafuzz.py %d %d%s\n' % (seed, count, ' (with prefix)' if prefix else ''))
        for c in prefix + gen(seed, count):
            f.write(c + '\n')


if __name__ == '__main__':
    main()
