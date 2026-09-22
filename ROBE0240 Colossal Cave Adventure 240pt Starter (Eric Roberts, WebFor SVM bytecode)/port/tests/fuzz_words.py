import re
TEXT = "D:/SynologyDrive/_/RECONSTRUCTIONS/ROBE0665 Colossal Cave Adventure 665pt Wellesley (Eric Roberts, FORTRAN source + WebFor SVM bytecode)/src_original/ROBE0665/text.dat"
# vocabulary: 5-letter chunks of the travel/vocabulary words in text.dat, plus common words
words = set("""take drop open close light off on wave pour eat drink rub throw quit find inventory feed fill kill
attack score look read break wake n s e w ne nw se sw u d in out enter xyzzy plugh plover lamp keys grate cage
bird rod food bottle water oil snake axe knife clam oyster magazine plant door troll bear chain gold diamonds
silver jewelry coins chest eggs trident vase emerald pyramid pearl rug spices help info brief save restore
all it with from into sack note poster matches honeycomb beehive pantry climb jump swim say get put""".split())
for line in open(TEXT, encoding="latin-1"):
    m = re.match(r"^\d+\s+\d+\s+\d+\s+(.*)$", line)
    if m:
        f = m.group(1)
        for i in range(0, len(f), 5):
            w = f[i:i + 5].strip().lower()
            if w.isalpha():
                words.add(w)
words = sorted(words - {"quit", "save", "restore"})
