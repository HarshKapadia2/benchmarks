CC = gcc
CFLAGS = -pedantic -Wall -Wextra -O3 -march=native -fopenmp

all: stream

install-dep:
	sudo apt-get install -y build-essential clang-format gcc gnuplot numactl

stream: src/stream.c run-stream
	$(CC) $(CFLAGS) src/stream.c -o stream
	chmod +x stream
	chmod +x run-stream

plot: stream-ccd-round-robin.csv stream-compact-cpu.csv plots/stream-ccd-round-robin.gnuplot plots/stream-compact-cpu.gnuplot
	gnuplot plots/stream-ccd-round-robin.gnuplot
	gnuplot plots/stream-compact-cpu.gnuplot

# Only for learning purposes
openmp-basics: src/learning/openmp-basics.c
	$(CC) $(CFLAGS) src/learning/openmp-basics.c -o openmp-basics
	chmod +x openmp-basics

clean:
	rm -f openmp-basics stream

format:
	find . -name "*.c" -or -name "*.h" | xargs clang-format -i

.PHONY: all install-dep clean format plot

