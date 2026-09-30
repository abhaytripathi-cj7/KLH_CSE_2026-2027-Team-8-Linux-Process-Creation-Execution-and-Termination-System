# Experiment 10: inodes, links, and memory mapped I/O

Open or edit the source with `nano mmap_io.c` and `nano inode_links.sh`.

```sh
bash inode_links.sh
gcc -std=c11 -Wall -Wextra -O2 mmap_io.c -o mmap_io
printf 'memory mapped copy\n' > input.txt
./mmap_io mmap input.txt mapped.txt
./mmap_io rw input.txt traditional.txt
cmp input.txt mapped.txt && cmp input.txt traditional.txt
```

`ls -i` and `stat` show that a hard link shares the original inode and raises its link count. A symbolic link has its own inode and stores a path; deleting the target leaves a dangling symbolic link. `find -samefile` lists names of the same inode. The C program reads and writes a file using `mmap` in one mode and `read`/`write` in the other. It times both, including `msync` or `fsync`, but timings depend on file size and caching. Memory mapping makes access look like memory operations, while traditional I/O uses explicit buffers and handles partial writes; mapping needs extra care for empty files and mapping failures.
