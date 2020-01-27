#! /bin/bash
echo 'NORMAL COPY TESTS'
error_c=0
var_o=`echo '114514' | ./lab0`
if [ $? -lt 0 ]
then
    echo "ERROR: failed copy from stdin to stdout"
    error_c=$error_c+1
fi
if [ "$var_o" = "114514" ]
then
    echo "114514"
else
    echo "ERROR: stdin stdout contents don't match"
    error_c=$error_c+1
fi

echo "114514" > smoke.log
var_o=`./lab0 --input=smoke.log`
if [ $? -lt 0 ]
then
    echo "ERROR: failed copy from file to stdout"
    error_c=$error_c+1
fi
if [ "$var_o" = "114514" ]
then
    echo "114514"
else
    echo "ERROR: input file stdout contents don't match"
    error_c=$error_c+1
fi
rm -f smoke.log

echo '114514' | ./lab0 --output=smoke.log
if [ $? -lt 0 ]
then
    echo "ERROR: failed copy from stdin to created file"
    error_c=$error_c+1
fi
var_o=`cat smoke.log`
if [ "$var_o" = "114514" ]
then
    echo "114514"
else
    echo "ERROR: stdin output file contents don't match"
    error_c=$error_c+1
fi
rm -f smoke.log

./lab0 --input=Makefile --output=smoke.log
if [ $? -lt 0 ]
then
    echo "ERROR: failed copy from file to created file"
    error_c=$error_c+1
fi
var_i=`cat Makefile`
var_o=`cat smoke.log`
if [ "$var_i" = "$var_o" ]
then
    echo "file to file pass"
else
    echo "ERROR: input file and output file contents don't match"
    error_c=$error_c+1
fi
rm -f smoke.log

./lab0 --input=Makefile --output=smoke.log --catch
if [ $? -lt 0 ]
then
    echo "ERROR: failed copy from file to created file when there's stand-alone catch option"
    error_c=$error_c+1
fi
rm -f smoke.log

echo ""
echo "SEG FAULT AND CATCHING"
./lab0 --segfault
if [ $? -ne 139 ]
then
    echo "ERROR: exit code from --segfault: $?, should be 139"
    error_c=$error_c+1
fi

./lab0 --segfault --catch
if [ $? -ne 4 ]
then
    echo "ERROR: exit code from --segfault --catch: $?, should be 4"
    error_c=$error_c+1
fi

./lab0 --input=Makefile --output=smoke.log --segfault --catch
if [ $? -ne 4 ]
then
    echo "ERROR: exit code from --segfault --catch: $?, should be 4"
    error_c=$error_c+1
fi
rm -f smoke.log

echo ""
echo "INVALID OPTION TESTS"
./lab0 --input=aloha
if [ $? -ne 2 ]
then 
    echo "ERROR: exit code from nonexistent input file: $?, should be 2"
    error_c=$error_c+1
fi

echo "114514" > smoke.log
chmod 000 smoke.log
./lab0 --output=smoke.log
if [ $? -ne 3 ]
then 
    echo "ERROR: exit code from non-writable file: $?, should be 3"
    error_c=$error_c+1
fi
rm -f smoke.log

./lab0 -a
if [ $? -ne 1 ]
then 
    echo "ERROR: exit code from invalid argument: $?, should be 1"
    error_c=$error_c+1
fi

./lab0 --input=Makefile 123
if [ $? -ne 1 ]
then 
    echo "ERROR: exit code from invalid argument: $?, should be 1"
    error_c=$error_c+1
fi

echo ""
echo "Smoke test finish. Total $error_c errors."
