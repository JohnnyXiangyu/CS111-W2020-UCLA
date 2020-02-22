cc=gcc
flags=-Wall -Wextra -g 

default: lab4b

lab4b: lab4b.c 
	$(cc) lab4b.c -o lab4b $(flags)

dist: lab4b.c Makefile README
	tar -cvzf lab4b-805061659.tar.gz lab4b.c Makefile README

check: lab4b
	bash lab4b_check.sh 

clean: 
	rm -f lab4b-805061659.tar.gz
	rm -f lab4b
