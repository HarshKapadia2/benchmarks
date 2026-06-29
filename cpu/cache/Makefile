CC = gcc
CFLAGS = -O2 -pedantic -Wall

all: install-dep arr-seq-stride arr-rand-ptr-chase

install-dep:
	sudo apt-get update
	sudo apt-get install -y libsodium23 libsodium-dev

arr-seq-stride: src/arr-seq-stride.c
	$(CC) $(CFLAGS) src/arr-seq-stride.c -o arr-seq-stride
	chmod +x arr-seq-stride

arr-rand-ptr-chase: src/arr-rand-ptr-chase.c
	$(CC) $(CFLAGS) src/arr-rand-ptr-chase.c -lsodium -lm -o arr-rand-ptr-chase
	chmod +x arr-rand-ptr-chase

clean:
	rm -f arr-seq-stride arr-rand-ptr-chase

format:
	find . -name "*.c" -or -name "*.h" | xargs clang-format -i

.PHONY: all install-dep clean format
