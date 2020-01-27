cc = gcc
flag = -g -Wall -Wextra
all: lab0

check: lab0
	sh smoke_test.sh

lab0: lab0.c
	$(cc) lab0.c -o lab0 $(flag)

dist: lab0.c Makefile README breakpoint.png backtrace.png smoke_test.sh
	tar -cvzf lab0-805061659.tar.gz lab0.c Makefile README breakpoint.png backtrace.png smoke_test.sh

clean:
	rm -f lab0
	rm -f lab0-805061659.tar.gz
