build/gamelog: build/ main.c
	gcc -Werror -o build/gamelog main.c -lsqlite3

build/:
	mkdir build

clean:
	rm -rf build

dependencies:
	apt install libsqlite3-dev

install: build/gamelog
	cp build/gamelog /usr/local/bin/gamelog
