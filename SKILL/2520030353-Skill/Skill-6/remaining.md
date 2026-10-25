# Skill 6 Part B: directory navigation and builtin state

The `cd` builtin in `../OSSP/src/builtin.c` uses `chdir`, checks errors, remembers the prior working directory, and supports `cd -`. It runs in the shell process so the next command sees the new location. The same dispatcher handles other builtins.

```text
pwd
cd /tmp
pwd
cd -
pwd
cd /no-such-directory
```

The final command prints an error, and the shell remains usable.
