cc=gcc
flags=-Wall -Wextra -g -lmraa -lm

lab=Lab_4b
files=lab4b.c Makefile README lab4b_check.sh

usr=root
host=beagle# this is configuration on my device only

default: lab4b

lab4b: lab4b.c 
	$(cc) lab4b.c -o lab4b $(flags)

dist: $(files)
	tar -cvzf lab4b-805061659.tar.gz $(files)

check: lab4b
	bash lab4b_check.sh 

install: $(files)
	scp $(files) P4B_check.sh $(host):/root/projects/cs111/$(lab)/

clean: 
	rm -f lab4b-805061659.tar.gz
	rm -f lab4b
