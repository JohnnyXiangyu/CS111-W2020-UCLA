#! /bin/bash

# Q2.3.1
echo "" > lab2b.csv

for i in 1 2 4 8 12 16 24
do
    ./lab2_list --threads=$i --iterations=1000 --sync=m >> lab2b.csv
    ./lab2_list --threads=$i --iterations=1000 --sync=s >> lab2b.csv
done
