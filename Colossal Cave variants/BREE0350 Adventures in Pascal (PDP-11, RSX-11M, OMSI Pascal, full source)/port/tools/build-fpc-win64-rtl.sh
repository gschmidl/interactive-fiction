#!/bin/bash
# Build the Free Pascal 3.2.2 Win64 run-time units with the Debian fpc package.
#
# Debian's ppcx64 can already target Win64 (-Twin64, internal assembler and
# linker, so no binutils), but the package only ships the Linux units, and
# fpc-source ships the RTL without its generated Makefiles.  This compiles the
# units straight from /usr/share/fpcsrc into ~/fpc-win64/units (no root needed).
#
#   sudo apt install fpc fpc-source      # once, if not already there
#   bash tools/build-fpc-win64-rtl.sh
set -e
SRC=${FPCSRC:-/usr/share/fpcsrc/3.2.2}
U=${FPC_WIN64_UNITS:-$HOME/fpc-win64/units}
WORK=$(mktemp -d "$HOME/fpc-win64-build.XXXXXX")
trap 'rm -rf "$WORK"' EXIT
cp -r "$SRC/rtl" "$WORK/rtl"
mkdir -p "$U"
cd "$WORK/rtl/win64"
OPT="-Twin64 -Px86_64 -O2 -Fi../inc -Fi../x86_64 -Fi../win -Fi../win/wininc -Fi../objpas -Fi../objpas/sysutils -Fi../objpas/classes -Fu../inc -Fu../x86_64 -Fu../win -Fu../objpas -Fu../objpas/sysutils -Fu../objpas/classes -Fu../common -Fu../charmaps -FU$U -vew"
ppcx64 $OPT -Us -Sg system.pp     > "$WORK/log" 2>&1 || { tail -20 "$WORK/log"; exit 1; }
for u in ../inc/uuchar.pp ../objpas/objpas.pp ../inc/iso7185.pp buildrtl.pp ../inc/lineinfo.pp ../inc/lnfodwrf.pp; do
    ppcx64 $OPT $u >> "$WORK/log" 2>&1 || { tail -30 "$WORK/log"; exit 1; }
done
echo "Win64 units in $U: $(ls "$U"/*.ppu | wc -l)"
