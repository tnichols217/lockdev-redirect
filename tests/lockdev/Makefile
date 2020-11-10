CC ?= gcc
CFLAGS ?= -g -O3 -Wall

OBJS = lockdev.o sample.o

all: sample

%.o: %.c
	$(CC) -c -o $@ $<

sample: $(OBJS)
	$(CC) -rdynamic $(OBJS) -o sample

test: all
	@echo Testing lock on /dev/ttyS3
	@./sample -l /dev/ttyS3 && echo "PASSED"
	@echo Testing relock on /dev/ttyS3
	@./sample -r /dev/ttyS3 && echo "PASSED"
	@echo Testing unlock on /dev/ttyS3
	@./sample -u /dev/ttyS3 && echo "PASSED"

clean:
	rm -f *.o
	rm -f sample
