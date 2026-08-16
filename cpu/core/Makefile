CC = gcc
CFLAGS = -O2 -pedantic -Wall -Wextra -pthread

all: ping-pong

install-dep:
	sudo apt-get install -y clang-format gcc gnuplot

ping-pong: src/ping-pong.c
	$(CC) $(CFLAGS) src/ping-pong.c -o ping-pong
	chmod +x ping-pong

plot-ping-pong-heatmap: plots/core-to-core-latency-heatmap.gnuplot core-to-core-latency.csv
	gnuplot plots/core-to-core-latency-heatmap.gnuplot

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
	rm -f ping-pong pthread-basics pthread-affinity atomics-basics

format:
	find . -name "*.c" -or -name "*.h" | xargs clang-format -i

.PHONY: all install-dep clean format

