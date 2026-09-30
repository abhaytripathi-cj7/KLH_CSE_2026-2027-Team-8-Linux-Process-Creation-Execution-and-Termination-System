# Skill 9: input, output, append, and stderr redirection

`../OSSP/src/parser.c` recognizes `<`, `>`, `>>`, `2>`, and `2>>`. `../OSSP/src/executor.c` opens files with the appropriate flags and uses `dup2`. Failed opens report an error; a standalone builtin restores descriptors after it finishes.

```text
echo first > log.txt
echo second >> log.txt
cat < log.txt
cat missing.txt 2> errors.txt
cat errors.txt
```

The append operation preserves the first line. The stderr example captures only the failed `cat` diagnostic.
