# Experiment 9: file I/O and descriptor redirection

Open or edit the source with `nano file_copy.c` and `nano redirect.c`.

```sh
gcc -std=c11 -Wall -Wextra -O2 file_copy.c -o file_copy
gcc -std=c11 -Wall -Wextra -O2 redirect.c -o redirect
printf 'OSSP practical 9\n' > input.txt
./file_copy sys input.txt sys.txt
./file_copy stdio input.txt stdio.txt
cmp input.txt sys.txt && cmp input.txt stdio.txt
./redirect input.txt redirected.txt
cmp input.txt redirected.txt
```

`file_copy` uses `open`, `lseek`, `read`, `write`, and `close` in `sys` mode, and `fopen`, `fread`, and `fwrite` in `stdio` mode. It reports elapsed time for each copy. For a useful timing comparison, use a larger regular file and repeat both modes; the page cache changes timing. `redirect` uses `dup2` to replace file descriptors 0 and 1 before copying from standard input to standard output, as a shell does for `<` and `>`.
