#!/bin/bash
# Build the port with Free Pascal 3.2.2 cross-compiling to Win64 (run inside WSL):
#     wsl -d Debian -- bash build.sh          release build, *.exe next to this file
#     wsl -d Debian -- bash build.sh debug    build/dbg/adventure-dbg.exe: line numbers
#                                             in run-time errors, range + overflow checks
# The Win64 RTL units are expected in ~/fpc-win64/units (tools/build-fpc-win64-rtl.sh).
set -e
cd "$(dirname "$0")"
UNITS=${FPC_WIN64_UNITS:-$HOME/fpc-win64/units}
mkdir -p build/gen build/obj build/dbg
python3 tools/weave.py ../src_original build/gen > build/weave.out
BASE="ppcx64 -Twin64 -Px86_64 -vew -Fu$UNITS -Fibuild/gen -Fisrc"
if [ "$1" = debug ]; then
    $BASE -gl -gw -Cr -Co -FUbuild/dbg -FEbuild/dbg -oadventure-dbg.exe src/adventure.pas
    exit 0
fi
FPC="$BASE -O2 -Xs -FUbuild/obj -FEbuild"
$FPC src/omsirt.pas
for p in adventure peek poof advfls; do $FPC -o$p.exe src/$p.pas; done
$FPC -o100fls.exe src/fls100.pas
cp build/adventure.exe build/peek.exe build/poof.exe build/advfls.exe build/100fls.exe .
ls -l *.exe
