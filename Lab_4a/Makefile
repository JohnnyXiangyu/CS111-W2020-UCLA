lab=lab_4a
host=192.168.137.157
lhost=192.168.7.2
usr=root

disst: build_run.png connected.png copy.png git.png my_beaglebone.jpg Makefile README
	tar -cvzf lab4a-805061659.tar.gz build_run.png connected.png copy.png git.png my_beaglebone.jpg Makefile README

install: helloworld.c
	scp ./helloworld.c $(usr)@$(host):/root/projects/cs111/$(lab)/

connect:
	ssh $(usr)@$(host)

lconnect:
	ssh $(usr)@$(lhost)
