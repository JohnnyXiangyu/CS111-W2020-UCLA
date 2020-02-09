#! /bin/bash

# for 1 and 2
for i in 2 4 8 12
do
    for j in 10 20 40 80 100 1000 10000 100000
    do
        ./lab2_add --threads=$i --iterations=$j --yield >> lab2_add.csv
        ./lab2_add --threads=$i --iterations=$j >> lab2_add.csv
    done
done

# for 3
for i in 1 10 100 1000 10000 100000
do
    ./lab2_add --threads=1 --iterations=$i >> lab2_add.csv
    ./lab2_add --threads=1 --iterations=$i --yield >> lab2_add.csv
done

# for 4
for i in 2 4 8 12
do
    ./lab2_add --threads=$i --iterations=10000 --yield --sync=c >> lab2_add.csv
    ./lab2_add --threads=$i --iterations=10000 --yield --sync=m >> lab2_add.csv
    ./lab2_add --threads=$i --iterations=1000 --yield --sync=s >> lab2_add.csv
    ./lab2_add --threads=$i --iterations=10000 --yield --sync=s >> lab2_add.csv
done

# for 5
for i in 1 2 4 8 12
do 
    ./lab2_add --threads=$i --iterations=10000 >> lab2_add.csv
    ./lab2_add --threads=$i --iterations=10000 --sync=s >> lab2_add.csv
    ./lab2_add --threads=$i --iterations=10000 --sync=m >> lab2_add.csv
    ./lab2_add --threads=$i --iterations=10000 --sync=c >> lab2_add.csv
done