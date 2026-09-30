# Experiment 12: producer consumer and deadlock

Open or edit the source with `nano producer_consumer.c` and `nano deadlock.c`.

```sh
gcc -std=c11 -Wall -Wextra -O2 -pthread producer_consumer.c -o producer_consumer
gcc -std=c11 -Wall -Wextra -O2 -pthread deadlock.c -o deadlock
./producer_consumer 1 100000
./producer_consumer 16 100000
./producer_consumer 256 100000
./deadlock deadlock
./deadlock prevent
```

The producer and consumer share a bounded circular buffer. `empty_slots` and `full_slots` are counting semaphores; a mutex protects the queue indices. Matching produced, consumed, and expected sums verify the run. Throughput varies with buffer size and machine load.

The deadlock mode makes two threads hold opposite mutexes and wait for each other. It runs in a child process that the parent stops after one second, so the demonstration does not hang the terminal. The four necessary conditions are mutual exclusion, hold and wait, no preemption, and circular wait. The prevention mode makes both threads acquire resources in the same order, removing circular wait.
