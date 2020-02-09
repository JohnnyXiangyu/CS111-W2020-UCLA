#! /usr/bin/gnuplot

# general plot parameters
set terminal png
set datafile separator ","

# testing artifact vs number of iterations
set title "Add-3: per operation cost vs number of iterations"
set xlabel "Iterations"
set logscale x 10
set ylabel "Cost per operation (ns)"
set logscale y 10
set output 'lab2_add-3.png'
plot "< grep 'add-none,1,' lab2_add.csv" using ($3):($6) \
	title 'Threads=1, w/o yields' with points
