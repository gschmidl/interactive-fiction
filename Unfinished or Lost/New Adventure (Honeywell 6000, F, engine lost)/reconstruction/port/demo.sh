#!/bin/sh
# A short guided walk: the road, the grate, the cavern, the mallorn grove,
# and the one door in the cave that Tolkien would have recognised.
./newadv -e <<'EOF'
exits
e
w
s
exits
d
go ABOVE_GRATE
exits
d
force
exits
path CAVE_TOP
go TREE_FRONT
e
exits
u
exits
d
go STONE_DOOR
read
e
acts
src FRIEND
reach
quit
EOF
