#!/usr/bin/env gnuplot

reset

#-----------------------------------------------------------------------------
# Output
#-----------------------------------------------------------------------------
set terminal pngcairo size 1600,900 enhanced font "Arial,14"
set output "stream-compact-cpu.png"

#-----------------------------------------------------------------------------
# Data
#-----------------------------------------------------------------------------
set datafile separator ","
DATAFILE = "stream-compact-cpu.csv"

#-----------------------------------------------------------------------------
# Appearance
#-----------------------------------------------------------------------------
set title "STREAM Triad Kernel Bandwidth - Compact CPU Placement"
set xlabel "No. of cores or threads (SMT off, so 1 thread per CPU core)"
set ylabel "Bandwidth (GB/s)"

set grid xtics ytics mytics
set border linewidth 1.5

set key top left

#-----------------------------------------------------------------------------
# X Axis
#-----------------------------------------------------------------------------
set xrange [1:64]
set xtics ( \
    "1" 1, \
    "2" 2, \
    "4" 4, \
    "8" 8, \
    "16" 16, \
    "24" 24, \
    "32" 32, \
    "40" 40, \
    "48" 48, \
    "56" 56, \
    "64" 64 \
)

#-----------------------------------------------------------------------------
# Y Axis
#-----------------------------------------------------------------------------
set yrange [0:*]
set ytics 10
set mytics 2

#-----------------------------------------------------------------------------
# Line Style
#-----------------------------------------------------------------------------
set style line 1 \
    lc rgb "#1f77b4" \
    lw 3 \
    pt 7 \
    ps 1.5

#-----------------------------------------------------------------------------
# Automatically determine peak
#-----------------------------------------------------------------------------
stats DATAFILE using 5 nooutput
peak_bw = STATS_max

stats DATAFILE using (($5 == peak_bw) ? $1 : 1/0) nooutput
peak_threads = STATS_min

set arrow 1 \
    from peak_threads,0 \
    to peak_threads,peak_bw \
    nohead \
    dt 2 \
    lw 1.5 \
    lc rgb "red"

set label 1 \
    sprintf("Peak: %.1f GB/s at %d threads", \
            peak_bw, int(peak_threads)) \
    at peak_threads-12, peak_bw+3 \
    tc rgb "red"

#-----------------------------------------------------------------------------
# Plot
# Column 5 = triad_gigabytespersec
#-----------------------------------------------------------------------------
plot DATAFILE using 1:5 with linespoints ls 1 title "Triad kernel bandwidth"
