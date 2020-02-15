#! /usr/bin/gnuplot

# general plot parameters
set terminal png
set datafile separator ","

# lab2b_1.png
set title "Operations per Second of Mutex/Spin Lock"
set xlabel "# of Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (ops/s)"
set logscale y 10
set output 'lab2b_1.png'

# single threaded, unprotected, no yield
plot \
     "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'list w/spin' with linespoints lc rgb 'green', \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'list w/mutex' with linespoints lc rgb 'red'
