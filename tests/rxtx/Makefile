CC ?= gcc
CFLAGS ?= -g -O3 -Wall

all: testrun

testrun: rxtx_test.c
	$(CC) rxtx_test.c -o testrun

test: all
	@./testrun

clean:
	rm -f testrun
