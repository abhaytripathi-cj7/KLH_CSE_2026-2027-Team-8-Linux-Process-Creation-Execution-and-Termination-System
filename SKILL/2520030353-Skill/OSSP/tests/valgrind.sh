#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
printf 'export DEMO=ready\npwd\nhistory\nexit 0\n' > "$tmp/commands.txt"
valgrind --quiet --leak-check=full --show-leak-kinds=all --error-exitcode=99 \
    "$root/ossp-shell" < "$tmp/commands.txt" > "$tmp/stdout.txt" 2> "$tmp/valgrind.txt"
if grep -qE 'definitely lost: [1-9]|invalid read|invalid write' "$tmp/valgrind.txt"; then
    cat "$tmp/valgrind.txt" >&2
    exit 1
fi
printf 'Valgrind memory check passed\n'
