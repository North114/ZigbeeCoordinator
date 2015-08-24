#!/bin/sh

a=10
b=20
for var in 0 1 2 3 4 5 6 7 8 9
do
   echo $var
done

NAME="Zara Ali"
#unset NAME 
echo $NAME
if [ $a != $b ]
then echo "a is not equal to b"
    #a=`expr $a - 1`
else echo "a is equal to b"
fi
