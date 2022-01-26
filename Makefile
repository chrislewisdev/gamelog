build/gamelog: build/ main.c
	gcc -o build/gamelog main.c -lsqlite3

build/:
	mkdir build

clean:
	rm -rf build

dependencies:
	apt install libsqlite3-dev