all: install-dep cache-line cache-size struct-cacheline

install-dep:
	sudo apt-get update
	sudo apt-get install -y libsodium23 libsodium-dev

cache-line: src/cache-line.c
	gcc -Wall -pedantic src/cache-line.c -o cache-line
	chmod +x cache-line

cache-size: src/cache-size.c
	gcc -Wall -pedantic src/cache-size.c -lsodium -lm -o cache-size
	chmod +x cache-size

struct-cacheline: src/struct-cacheline.c
	gcc -Wall -pedantic src/struct-cacheline.c -lsodium -o struct-cacheline
	chmod +x struct-cacheline

clean:
	rm -f cache-line cache-size struct-cacheline

format:
	find . -name "*.c" -or -name "*.h" | xargs clang-format -i

.PHONY: all install-dep clean format
