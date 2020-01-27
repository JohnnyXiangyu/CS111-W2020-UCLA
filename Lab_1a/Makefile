cc=gcc
flags=-Wall -Wextra -g

all: lab1a

dist: lab1a
	tar -cvzf lab1a-805061659.tar.gz lab1a.c Makefile README

lab1a: lab1a.c
	$(cc) lab1a.c -o lab1a $(flags)

clean:
	rm -f lab1a
	rm -f *.tar.gz
