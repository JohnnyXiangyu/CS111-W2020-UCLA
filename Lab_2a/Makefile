cc=gcc
flags=-g -Wall -Wextra -lpthread

build: lab2_add lab2_list

tests: lab2_add
	bash add_tests.sh

graphs: lab2_add.csv
	gnuplot lab2_add.gp

lab2_add.csv: tests

lab2_list.csv: tests

dist:

lab2_add: lab2_add.c
	$(cc) lab2_add.c -o lab2_add $(flags)

lab2_list: lab2_list.c SortedList.h SortedList.c 
	$(cc) lab2_list.c SortedList.c -o lab2_list $(flags)

clean:
	rm -f lab2_add
	rm -f lab2_list
	rm -f *.png
	rm -f *.csv
