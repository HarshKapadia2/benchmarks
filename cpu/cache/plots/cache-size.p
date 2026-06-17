set terminal x11

set grid linetype 7 linecolor rgb "#e1f5fe"

set key left

set tics nomirror
set xtics offset 0,-1

set logscale x 2
set format x '2^{%L}'
 
set xlabel "Array size (B)" offset 0,-1
set ylabel "Avg. single element access time (ns)"
 
# Add vertical lines to mark cache sizes
# Determine max y value (for GPVAL_Y_MAX) by plotting to a dummy terminal
set terminal push
set terminal unknown
plot "sample-data/cache-size.dat" using 2
set terminal pop
# Mark L1d, L2, and L3
maxy = GPVAL_Y_MAX
l1 = 32768 # in bytes
l2 = 524288
l3 = 33554432
set arrow from l1,0 to l1,maxy nohead linecolor rgb "blue"
set arrow from l2,0 to l2,maxy nohead linecolor rgb "blue"
set arrow from l3,0 to l3,maxy nohead linecolor rgb "blue"

plot "sample-data/cache-size.dat" using 1:2 with \
	linespoints \
	linetype 7 \
	linecolor "black" \
	title "AMD EPYC 7713 (L1d: 32 kB, L2: 512 kB, L3: 32 MB)"
