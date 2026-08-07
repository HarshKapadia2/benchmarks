CC = gcc
CFLAGS = -O2 -pedantic -Wall -Wextra -pthread

all:

# Only for learning purposes
pthread-basics: src/learning/pthread-basics.c
	$(CC) $(CFLAGS) src/learning/pthread-basics.c -o pthread-basics
	chmod +x pthread-basics

# Only for learning purposes
pthread-affinity: src/learning/pthread-affinity.c
	$(CC) $(CFLAGS) src/learning/pthread-affinity.c -o pthread-affinity
	chmod +x pthread-affinity

# Only for learning purposes
atomics-basics: src/learning/atomics-basics.c
	$(CC) $(CFLAGS) src/learning/atomics-basics.c -o atomics-basics
	chmod +x atomics-basics

clean:
	rm -f pthread-basics pthread-affinity atomics-basics

install-formatter:
	sudo apt-get install -y clang-format

format:
	find . -name "*.c" -or -name "*.h" | xargs clang-format -i

.PHONY: all clean install-formatter format

