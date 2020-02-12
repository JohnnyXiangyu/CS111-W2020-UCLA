#! /bin/bash

echo "" > lab2_list.csv

# for 1
for i in 10 100 1000 10000 20000
do
    ./lab2_list --threads=1 --iterations=$i >> lab2_list.csv
done

# for 2
for i in 2 4 8 12
do 
    for j in 1 10 100 1000
    do 
        ./lab2_list --threads=$i --iterations=$j >> lab2_list.csv
    done
done

for i in 2 4 8 12
do 
    for j in 1 2 4 8 16 32
    do 
        ./lab2_list --threads=$i --iterations=$j --yield=i >> lab2_list.csv
        ./lab2_list --threads=$i --iterations=$j --yield=d >> lab2_list.csv
        ./lab2_list --threads=$i --iterations=$j --yield=il >> lab2_list.csv
        ./lab2_list --threads=$i --iterations=$j --yield=dl >> lab2_list.csv
    done
done

# for 3
./lab2_list --threads=12 --iterations=32 --yield=i  --sync=s>> lab2_list.csv
./lab2_list --threads=12 --iterations=32 --yield=d  --sync=s>> lab2_list.csv
./lab2_list --threads=12 --iterations=32 --yield=il --sync=s>> lab2_list.csv
./lab2_list --threads=12 --iterations=32 --yield=dl --sync=s>> lab2_list.csv
./lab2_list --threads=12 --iterations=32 --yield=i  --sync=m>> lab2_list.csv
./lab2_list --threads=12 --iterations=32 --yield=d  --sync=m>> lab2_list.csv
./lab2_list --threads=12 --iterations=32 --yield=il --sync=m>> lab2_list.csv
./lab2_list --threads=12 --iterations=32 --yield=dl --sync=m>> lab2_list.csv

# for 4
for i in 1 2 4 8
do 
    ./lab2_list --threads=$i --iterations=1000 --sync=s>> lab2_list.csv
    ./lab2_list --threads=$i --iterations=1000 --sync=m>> lab2_list.csv
done
