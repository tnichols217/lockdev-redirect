CC ?= gcc
CFLAGS ?= -g -O3 -Wall

all: testrun

testrun: custom_test.c
	$(CC) custom_test.c -D_LARGEFILE64_SOURCE -o testrun

test: all
	@./testrun

clean:
	rm -f testrun
