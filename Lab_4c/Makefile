cc=gcc
flags=-Wall -Wextra -g -lmraa -lm 

lab=Lab_4b
files=lab4c.c Makefile README lab4c_check.sh

usr=root
host=beagle# this is configuration on my device only

default: lab4c_tcp lab4c_tls

lab4c_tcp: lab4c_tcp.c 
	$(cc) lab4c_tcp.c -o lab4c_tcp $(flags)

lab4c_tls: lab4c_tls.c 
	$(cc) lab4c_tls.c -o lab4c_tls $(flags) -lssl -lcrypto

dist: $(files)
	tar -cvzf lab4c-805061659.tar.gz $(files)

check: lab4c
	bash lab4c_check.sh 

install: $(files)
	scp $(files) P4C_check.sh $(host):/root/projects/cs111/$(lab)/

clean: 
	rm -f lab4c-805061659.tar.gz
	rm -f lab4c_tcp
	rm -f lab4c_tls

update:
	git fetch
	git pull
	make
