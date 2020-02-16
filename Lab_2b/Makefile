cc=gcc
flags=-g -Wall -Wextra -lpthread -lprofiler 

build: lab2_list

tests: lab2_list
	bash lab2b_tests.sh

graphs: tests
	gnuplot lab2b.gp

lab2_list: lab2_list.c SortedList.h SortedList.c 
	$(cc) lab2_list.c SortedList.c -o lab2_list $(flags)

dist: profile Makefile graphs lab2b_tests README
	tar -czvf lab2b-805061659.tar.gz lab2_list.c \
	SortedList.h SortedList.c Makefile lab2_list.csv \
	lab2b_list-1.png lab2b_list-2.png lab2b_list-3.png \
	lab2b_list-4.png README lab2b.gp \
	lab2b_tests.sh profile.out

clean:
	rm -f lab2_list
	rm -f lab2b-805061659.tar.gz

dclean:
	rm -f *.csv
	rm -f *.png
	rm -f *.prof

fclean: dclean clean

profile: build
	rm -f ./temp.prof profile.out
	LD_PRELOAD=libprofiler.so.0 CPUPROFILE=./temp.prof ./lab2_list --threads=12 --iterations=1000 --sync=s
	pprof --text ./lab2_list ./temp.prof > profile.out
	pprof --list=threadRoutine ./lab2_list ./temp.prof >> profile.out
	rm -f ./temp.prof lab2_list
