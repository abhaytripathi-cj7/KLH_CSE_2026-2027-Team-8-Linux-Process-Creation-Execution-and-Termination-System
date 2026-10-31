.PHONY: run test check
run:
	bash start.sh
test:
	python3 -m unittest discover -s tests -v
check:
	gcc -std=c11 -O2 -Wall -Wextra -Werror -fsyntax-only backend/peer.c
	python3 -m py_compile backend/server.py
	bash -n start.sh START_UBUNTU.sh scripts/setup_ubuntu.sh scripts/inspect.sh
