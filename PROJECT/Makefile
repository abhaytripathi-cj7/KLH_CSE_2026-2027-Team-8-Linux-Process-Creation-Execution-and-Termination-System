CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2

.PHONY: all test clean
all: payment

payment: payment.c
	$(CC) $(CFLAGS) payment.c -o payment

test: payment
	python3 tests.py

clean:
	rm -f payment
