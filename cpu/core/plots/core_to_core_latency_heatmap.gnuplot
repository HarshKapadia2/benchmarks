#
# Core-to-Core Latency Heatmap
#
# Gnuplot plot file
#

INPUT  = "core-to-core-latency.csv"
OUTPUT = "core-to-core-latency-heatmap.png"

set datafile separator ","

#
# Determine number of cores.
#
# CSV contains:
#   1 header row
#   N^2 data rows
#
stats INPUT using 3 every ::1 nooutput

records   = STATS_records
data_rows = records
cores     = int(sqrt(data_rows) + 0.5)

if (cores * cores != data_rows) {
    print sprintf("ERROR: %d data rows is not a perfect square", data_rows)
    exit
}

# We want to ignore zeroes with an undefined value for color scaling
stats INPUT using ($3 > 0 ? $3 : 1/0) every ::1 nooutput

#
# Latency range
#
latency_min = STATS_min
latency_max = STATS_max

latency_range = latency_max - latency_min

if (latency_range <= 0) {
    latency_range = 1
}

threshold(v) = (v - latency_min) / latency_range
# label_threshold = 0.70
label_threshold = 0.55

#
# Dynamic image sizing
#
pixels_per_cell = 24

if (cores > 32)  pixels_per_cell = 18
if (cores > 64)  pixels_per_cell = 14
if (cores > 96)  pixels_per_cell = 10
if (cores > 128) pixels_per_cell = 8
if (cores > 192) pixels_per_cell = 6

image_size = int(cores * pixels_per_cell)

if (image_size < 1400) {
    image_size = 1400
}

set terminal pngcairo enhanced \
    size image_size,image_size \
    font "Arial,10"

set output OUTPUT

unset key

set title sprintf("Core-to-Core Latency Heatmap (%d Cores)", cores)

set size square

set xrange [-0.5:cores-0.5]
set yrange [cores-0.5:-0.5]

#
# Tick marks
#
if (cores <= 64) {
    #
    # Core numbers on top
    #
    unset xtics
    set x2tics 0,1,cores-1

    #
    # Core numbers on left
    #
    set ytics 0,1,cores-1

} else {
    unset xtics
    unset x2tics
    unset ytics
}

#
# Viridis palette
#
set palette viridis

set cbrange [latency_min:latency_max]

set colorbox
set cblabel "Latency (ns)"

#
# Display labels only on reasonably small matrices
#
show_labels = (cores <= 64)

fmt(v) = sprintf("%.0f", v)

#
# Grey diagonal cells
#
do for [i=0:cores-1] {
    set object (1000+i) rect \
        from i-0.5, i-0.5 \
        to   i+0.5, i+0.5 \
        fc rgb "#A0A0A0" \
        fillstyle solid 1.0 \
        noborder \
        front
}

if (show_labels) {

    plot \
        INPUT using 2:1:3 every ::1 with image notitle, \
        INPUT using \
            ($3==0 ? $2 : 1/0):\
            ($3==0 ? $1 : 1/0) \
            with points pt 5 ps 3 lc rgb "#B0B0B0" notitle, \
        INPUT using \
            2:1:( ($3 == 0 || threshold($3) < label_threshold) ? fmt($3) : "" ) \
            every ::1 \
            with labels center tc rgb "white" font ",8" notitle, \
        INPUT using \
            2:1:( ($3 == 0 || threshold($3) >= label_threshold) ? fmt($3) : "" ) \
            every ::1 \
            with labels center tc rgb "black" font ",8" notitle

} else {

    plot \
        INPUT using 2:1:3 every ::1 with image notitle

}

