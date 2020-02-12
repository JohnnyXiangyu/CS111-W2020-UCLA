cc=gcc
flags=-g -Wall -Wextra -lpthread

build: lab2_add lab2_list

tests: lab2_add.csv lab2_list.csv

graphs: lab2_add.csv lab2_list.csv
	gnuplot lab2_add.gp
	gnuplot lab2_list.gp

lab2_add.csv: add_tests.sh lab2_add
	bash add_tests.sh

lab2_list.csv: list_tests.sh lab2_list
	bash list_tests.sh

lab2_add: lab2_add.c
	$(cc) lab2_add.c -o lab2_add $(flags)

lab2_list: lab2_list.c SortedList.h SortedList.c 
	$(cc) lab2_list.c SortedList.c -o lab2_list $(flags)

dist: lab2_add.c SortedList.h SortedList.c lab2_list.c Makefile tests graphs list_tests.sh add_tests.sh README
	tar -czvf lab2a-805061659.tar.gz lab2_add.c lab2_list.c \
	SortedList.h SortedList.c Makefile lab2_add.csv lab2_list.csv \
	lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png \
	lab2_add-5.png lab2_list-1.png lab2_list-2.png lab2_list-3.png \
	lab2_list-4.png README lab2_add.gp lab2_list.gp \
	list_tests.sh add_tests.sh

clean:
	rm -f lab2_add
	rm -f lab2_list
	rm -f lab2a-805061659.tar.gz

dclean:
	rm -f *.csv
	rm -f *.png

fclean: dclean clean
