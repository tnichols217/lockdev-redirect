CC=gcc
CFLAGS=-Wall
LDFLAGS=

OBJS = utilities.o functions.o

all: lockdev-redirect.so

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

lockdev-redirect.so: $(OBJS)
	$(CC) -shared $(OBJS) -o lockdev-redirect.so -ldl $(LDFLAGS)

test:
	./full-testrun.sh

clean:
	rm -f lockdev-redirect.so
	rm -f *.o

	cd tests/rxtx && $(MAKE) clean
	cd tests/lockdev && $(MAKE) clean
