# Skill 13: integration, errors, memory, and tests

The modules connect through headers in `../OSSP/include`. Syntax errors and runtime failures print diagnostics without ending the shell. `make -C ../OSSP test` runs smoke, stress, and pseudo-terminal job-control tests. `bash ../OSSP/tests/valgrind.sh` checks a builtin-heavy session for invalid memory access and leaks when Valgrind is installed.

The Valgrind check passed on Ubuntu during development. See `../OSSP/docs/design.md` for module roles and known limits.
