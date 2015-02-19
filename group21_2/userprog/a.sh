#!/bin/bash
for i in {20..130}; 
do 
./nachos -F ../test/a.txt $i > c.txt ;
echo $i;
done