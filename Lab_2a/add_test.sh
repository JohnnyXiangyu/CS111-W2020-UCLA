#! /bin/bash
for i in {1..10001..10}
do
    ./lab2_add --threads=1 --iterations=$i >> lab2_add.csv
done
