#!/bin/sh
# Regenerate transcript.txt.
cd "$(dirname "$0")"

echo "=== 1. The Mars spine: ship -> Viking -> boulder -> airlock -> domed city ==="
printf '1\nlamp\nd\nw\nw\nw\nw\nw\nd\nd\nd\nd\nd\nd\nn\nne\nquit\nq\n' | ./mars.exe
echo
echo "=== 2a. The four B-engine bugs, exactly as written ==="
printf '1\nsouth\nread magazine\nlamp off\nquit\nq\n' | ./mars.exe
echo
echo "=== 2b. The same four commands with -fix ==="
printf '1\nsouth\nread magazine\nlamp off\nlamp on\nquit\nq\n' | ./mars.exe -fix
echo
echo "=== 3. The MIDJET puzzle in the adventure language, with -fix ==="
printf '3\nup\nw\nlogon\ncrash\nno\ns\nd\nd\nw\nn\nquit\nq\n' | ./mars.exe -fix
