CC=gcc
CFLAGS=-Wall
LDFLAGS=

lockdev-redirect.so: preload.c
	$(CC) $(CFLAGS) -shared -fPIC functions.c -o lockdev-redirect.so -ldl $(LDFLAGS)

test:
	./full-testrun.sh

clean:
	rm -f lockdev-redirect.so

	cd tests/rxtx && $(MAKE) clean
	cd tests/lockdev && $(MAKE) clean
