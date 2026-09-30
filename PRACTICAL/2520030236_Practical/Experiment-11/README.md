# Experiment 11: race condition and mutex

Open or edit the source with `nano counter.c`.

```sh
gcc -std=c11 -Wall -Wextra -O0 -pthread counter.c -o counter
./counter race 4 100000
./counter mutex 4 100000
```

The race mode deliberately performs unsynchronized read, increment, and write operations on a shared counter. Its final count can be less than the expected count and can vary by run. This deliberate data race has undefined behavior in C and is for demonstration only. The mutex mode guards each increment; its result should equal `THREADS * ITERATIONS`. Synchronization trades some speed for correctness. Try multiple runs and thread counts to compare results.
