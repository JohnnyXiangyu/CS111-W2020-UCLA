lab=lab_4a
host=192.168.137.157
lhost=192.168.7.2
usr=root

install: helloworld.c
	scp ./helloworld.c $(usr)@$(host):/root/projects/cs111/$(lab)/

connect:
	ssh $(usr)@$(host)

lconnect:
	ssh $(usr)@$(lhost)
