cc=gcc
flags=-g -Wall -Wextra -lpthread -lprofiler 

build: lab2_list

tests: lab2_list
	bash lab2b_tests.sh

graphs: tests
	gnuplot lab2b.gp

lab2_list: lab2_list.c SortedList.h SortedList.c 
	$(cc) lab2_list.c SortedList.c -o lab2_list $(flags)

dist: Makefile README graphs profile 
	tar -czvf lab2b-805061659.tar.gz SortedList.h SortedList.c lab2_list.c\
	Makefile\
	lab2b_list.csv profile.out\
	lab2b_1.png\
	lab2b_2.png\
	lab2b_3.png\
	lab2b_4.png\
	lab2b_5.png\
	README\
	lab2b_tests.sh lab2b.gp

clean:
	rm -f lab2_list
	rm -f lab2b-805061659.tar.gz

dclean:
	rm -f *.csv
	rm -f *.png
	rm -f *.prof
	rm -f *.out

fclean: dclean clean

profile: build
	rm -f ./temp.prof profile.out
	LD_PRELOAD=libprofiler.so.0 CPUPROFILE=./temp.prof ./lab2_list --threads=12 --iterations=1000 --sync=s
	pprof --text ./lab2_list ./temp.prof > profile.out
	pprof --list=threadRoutine ./lab2_list ./temp.prof >> profile.out
	rm -f ./temp.prof
