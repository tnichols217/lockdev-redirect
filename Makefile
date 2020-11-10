

lockdev-redirect.so: preload.c
	gcc -shared -fPIC preload.c -o lockdev-redirect.so -ldl

test:
	./full-testrun.sh

clean:
	rm -f lockdev-redirect.so

	cd tests/rxtx && $(MAKE) clean
	cd tests/lockdev && $(MAKE) clean
