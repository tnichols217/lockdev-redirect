CC?=gcc
CFLAGS?=-Wall
LDFLAGS?=

BINDIR=/usr/bin
LIBDIR=/usr/lib
DESTDIR=

OBJS = utilities.o functions.o

all: lockdev-redirect.so

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

lockdev-redirect.so: $(OBJS)
	$(CC) -shared $(OBJS) -o lockdev-redirect.so -ldl $(LDFLAGS)

install:
	install -D -m 755 lockdev-redirect.so $(DESTDIR)$(LIBDIR)/lockdev-redirect.so
	install -D -m 755 lockdev-redirect $(DESTDIR)$(BINDIR)/lockdev-redirect

test:
	./full-testrun.sh

clean:
	rm -f lockdev-redirect.so
	rm -f *.o

	cd tests/rxtx && $(MAKE) clean
	cd tests/lockdev && $(MAKE) clean
