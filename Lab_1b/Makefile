cc=gcc
flags=-Wall -Wextra -g -lz

all: lab1b-server lab1b-client

dist: lab1b-server.c lab1b-client.c Makefile README
	tar -cvzf lab1b-805061659.tar.gz lab1b-server.c lab1b-client.c Makefile README

lab1b-server: lab1b-server.c
	$(cc) lab1b-server.c -o lab1b-server $(flags)

lab1b-client: lab1b-client.c
	$(cc) lab1b-client.c -o lab1b-client $(flags)

clean:
	rm -f lab1b-server
	rm -f lab1b-client
	rm -f *.tar.gz
