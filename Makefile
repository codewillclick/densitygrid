
CC := gcc
OPTS := -02

all: clean subs compile

clean:
	-rm -r build

compile:
	-mkdir build
	$(CC) src/test.c -o build/test

subs: bmper

bmper:
	$(MAKE) -C src/bmper clean
	$(MAKE) -C src/bmper

test:
	build/test
