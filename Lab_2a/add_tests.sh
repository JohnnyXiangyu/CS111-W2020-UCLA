#! /bin/bash

echo "" > lab2_add.csv

# for 1
for i in 2 4 8 12
do
    for j in 100 1000 10000 100000
    do
        ./lab2_add --threads=$i --iterations=$j >> lab2_add.csv
    done
done

# for 2
for i in 2 4 8 12
do
    for j in 10 20 40 80 100 1000 10000 100000
    do
        ./lab2_add --threads=$i --iterations=$j --yield >> lab2_add.csv
    done
done

# for 3
for j in 1 2 4
do
    for i in 100 1000 10000 100000 1000000
    do
        ./lab2_add --threads=$j --iterations=$i >> lab2_add.csv
        ./lab2_add --threads=$j --iterations=$i --yield >> lab2_add.csv
    done
done

# for 5
for i in 1 2 4 8
do
    ./lab2_add --threads=$i --iterations=10000 --sync=s >> lab2_add.csv
    ./lab2_add --threads=$i --iterations=10000 --sync=m >> lab2_add.csv
    ./lab2_add --threads=$i --iterations=10000 --sync=c >> lab2_add.csv
    ./lab2_add --threads=$i --iterations=10000 --sync=s --yield >> lab2_add.csv
    ./lab2_add --threads=$i --iterations=10000 --sync=m --yield >> lab2_add.csv
    ./lab2_add --threads=$i --iterations=10000 --sync=c --yield >> lab2_add.csv
done
