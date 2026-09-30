# Skill 5 Part B: expansion and builtin dispatch

The completed implementation is in `../OSSP/src/parser.c` and `../OSSP/src/builtin.c`. The parser expands `$NAME` and `${NAME}` outside single quotes, preserves quoted spaces, turns undefined names into empty values, and recursively expands environment values up to eight levels. The builtin dispatcher recognizes `cd`, `pwd`, `export`, `history`, `jobs`, `fg`, `bg`, `exit`, and `help`; unsupported commands pass to `execvp`.

Run `make -C ../OSSP` from this folder, then `../OSSP/ossp-shell` and enter:

```text
export NAME=Linux
export NESTED='$NAME shell'
echo "$NESTED"
echo '$NAME'
help
```

Expected: `Linux shell`, then literal `$NAME`, then the builtin list.
