#!/bin/sh
# Build the native Adventure.  Needs a C compiler and zlib; on Windows the
# Strawberry Perl mingw-w64 toolchain has both.
set -e
cd "$(dirname "$0")"

CC=${CC:-gcc}
PACK=${PACK:-../src_original/advent-work.rk05}

# Regenerate the embedded pack only when it is missing or older than the source.
if [ ! -f src/rk05image.h ] || [ "$PACK" -nt src/rk05image.h ]; then
    python tools/mkimage.py "$PACK" src/rk05image.h
fi

$CC -O2 -Wall -Wextra -o adventure.exe src/pdp8.c -Isrc -static -lz
echo "built adventure.exe"
