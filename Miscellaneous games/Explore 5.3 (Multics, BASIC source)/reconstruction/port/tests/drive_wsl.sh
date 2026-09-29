#!/bin/sh
# drive_wsl.sh LIB SHARE SEED < commands > transcript
# drive.pl on the Linux Perl of WSL, in a scratch folder of its own.
d=$(mktemp -d)
perl "$(dirname "$0")/drive.pl" "$1" "$2" "$d" "$3"
s=$?
rm -rf "$d"
exit $s
