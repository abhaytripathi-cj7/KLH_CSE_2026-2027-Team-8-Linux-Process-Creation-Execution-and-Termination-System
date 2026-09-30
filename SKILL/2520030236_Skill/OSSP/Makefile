CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2 -Iinclude
SOURCES = src/main.c src/shell.c src/parser.c src/builtin.c src/executor.c

.PHONY: all test clean
all: ossp-shell
ossp-shell: $(SOURCES) include/shell.h include/parser.h include/builtin.h include/executor.h
	$(CC) $(CFLAGS) $(SOURCES) -o $@
test: ossp-shell
	bash tests/smoke.sh
	bash tests/stress.sh
	bash tests/history.sh
	python3 tests/job_control.py
clean:
	rm -f ossp-shell
