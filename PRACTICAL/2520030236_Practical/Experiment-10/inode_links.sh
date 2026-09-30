#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p run
printf 'inode demonstration\n' > run/original.txt
ln -f run/original.txt run/hard.txt
ln -sfn original.txt run/symbolic.txt
printf '%s\n' 'Inode numbers:'
ls -i run/original.txt run/hard.txt run/symbolic.txt
printf '%s\n' 'File metadata:'
stat -c '%n inode=%i type=%F links=%h' run/original.txt run/hard.txt run/symbolic.txt
printf '%s\n' 'Find all names of the original inode:'
find run -maxdepth 1 -samefile run/original.txt -print
