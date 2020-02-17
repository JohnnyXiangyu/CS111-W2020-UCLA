#! /bin/bash

# Q2.3.1
echo "" > lab2b_list.csv

for i in 1 2 4 8 12 16 24
do
    ./lab2_list --threads=$i --iterations=1000 --sync=m >> lab2b_list.csv # same line used for 2.3.3
    ./lab2_list --threads=$i --iterations=1000 --sync=s >> lab2b_list.csv
done

# Q2.3.4
for i in 1 2 4 8 12 16 
do
    for j in 1 2 4 8 16
    do 
        ./lab2_list --threads=$i --iterations=$j --yield=id --lists=4 >> lab2b_list.csv
    done

    for j in 10 20 40 80
    do 
        ./lab2_list --threads=$i --iterations=$j --yield=id --lists=4 --sync=m >> lab2b_list.csv
        ./lab2_list --threads=$i --iterations=$j --yield=id --lists=4 --sync=s >> lab2b_list.csv
    done
done
