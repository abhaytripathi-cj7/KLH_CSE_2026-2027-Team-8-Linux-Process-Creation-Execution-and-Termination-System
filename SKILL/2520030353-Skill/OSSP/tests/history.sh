#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
for n in $(seq 1 105); do printf 'echo item%d > /dev/null\n' "$n"; done > "$tmp/commands.txt"
printf 'history\nexit 0\n' >> "$tmp/commands.txt"
"$root/ossp-shell" < "$tmp/commands.txt" > "$tmp/history.txt"
test "$(wc -l < "$tmp/history.txt")" -eq 100
grep -q 'item7' "$tmp/history.txt"
if grep -q 'item6 ' "$tmp/history.txt"; then exit 1; fi
printf 'history capacity test passed\n'
