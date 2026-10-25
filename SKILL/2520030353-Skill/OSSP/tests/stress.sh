#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
cd "$tmp"
cat > commands.txt <<'COMMANDS'
export ROOT=hello
export INNER='$ROOT'
echo "$INNER" | tr a-z A-Z | cat | cat | cat > pipeline.txt
echo long | cat | cat | cat | cat | cat | cat | cat | cat | cat | cat | cat | cat | cat | cat | cat > long.txt
pwd > before.txt
cd /
pwd
cd -
pwd > after.txt
echo first > append.txt
echo second >> append.txt
cat no-such-file 2> errors.txt
echo broken |
export 1BAD=value
exit 0
COMMANDS
"$root/ossp-shell" < commands.txt > stdout.txt 2> stderr.txt
grep -qx HELLO pipeline.txt
grep -qx long long.txt
grep -qx / stdout.txt
cmp before.txt after.txt
printf 'first\nsecond\n' > expected.txt
cmp expected.txt append.txt
grep -q 'no-such-file' errors.txt
grep -q 'syntax:' stderr.txt
grep -q 'invalid variable name' stderr.txt
printf 'shell stress tests passed\n'
