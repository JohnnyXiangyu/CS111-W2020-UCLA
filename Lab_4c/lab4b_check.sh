#! /bin/bash

printf "LOG lol\nSCALE=F\nSCALE=C\nSTART\nSTOP\nOFF\n" | ./lab4b --log=smoke.log
for c in "LOG lol" SCALE=F SCALE=C START STOP OFF "SHUTDOWN"
do
    grep "$c" smoke.log > /dev/null
    if [ $? -ne 0 ]
    then
        echo "$c not logged"
    else
        echo "$c safe!"
    fi
done 

rm -f smoke.log
