
CC := gcc
OPTS := -02

all: clean subs compile

clean:
	-rm -r build

compile:
	-mkdir build
	cp src/*.c src/*.h build/
	cp src/bmper/build/bmper.h build/
	$(CC) build/test.c -o build/test

subs: bmper

bmper:
	$(MAKE) -C src/bmper clean
	$(MAKE) -C src/bmper

test:
	build/test
