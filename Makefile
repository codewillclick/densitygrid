
CC := gcc
OPTS := -02
PULL :=

all: clean subs compile

clean:
	-rm -r build

compile:
	-mkdir build
	cp src/*.c src/*.h build/
	cp src/bmper/build/bmper.h build/
	$(CC) build/test.c -o build/test
	$(CC) build/bmplot.c -o build/bmplot

subs: bmper

bmper:
	$(MAKE) -C src/bmper clean
	$(MAKE) -C src/bmper $(PULL) all

subpull:
	$(eval PULL := pull)

test:
	build/test
