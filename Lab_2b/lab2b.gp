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


# lab2b_2.png
set title "Avg Time spent on Mutex Locking"
set xlabel "# of Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Time/lock (ns)"
set logscale y 10
set output 'lab2b_2.png'

# single threaded, unprotected, no yield
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) \
	title 'per-op time' with linespoints lc rgb 'green', \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($8) \
	title 'wait-for-lock time' with linespoints lc rgb 'red'

# lab2b_3.png
set title "Corretness of Sub-list Implementation"
set logscale x 2
set xrange [0.75:]
set xlabel "# of Threads"
set ylabel "Succeeded # of Iterations"
set logscale y 10
set output 'lab2b_3.png'
plot \
    "< grep 'list-id-none,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'Unprotected'with points lc rgb 'green', \
    "< grep 'list-id-s,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'Spin-Lock' with points lc rgb 'red', \
    "< grep 'list-id-m,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'Mutex' with points lc rgb 'blue' 
