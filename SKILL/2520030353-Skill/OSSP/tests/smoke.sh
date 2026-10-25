#!/usr/bin/env bash
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
cd "$tmp"
cat > commands.txt <<'COMMANDS'
export NAME=hello
export NESTED='$NAME world'
echo "$NESTED"
echo one > append.txt
echo two >> append.txt
cat < append.txt | tr a-z A-Z > upper.txt
sh -c 'echo normal; echo error >&2' > combined.txt 2>&1
cat missing.txt 2> error.txt
sleep 1 &
jobs
fg
history
exit 0
COMMANDS
"$repo/ossp-shell" < commands.txt > stdout.txt 2> stderr.txt
grep -qx 'hello world' stdout.txt
printf 'ONE\nTWO\n' > expected.txt
cmp expected.txt upper.txt
grep -qx 'normal' combined.txt
grep -qx 'error' combined.txt
grep -q 'missing.txt' error.txt
grep -q 'Running sleep 1 &' stdout.txt
grep -q 'export NAME=hello' stdout.txt
printf 'shell smoke tests passed\n'
