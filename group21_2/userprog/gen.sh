for i in `seq 0 3`
do
for j in `seq 1 10`
do
if [ $i -eq 0 ]
then
	echo $j >> batch$(($i+1))_$j.txt
	for k in `seq 1 10`
	do
	echo ../test/testloop 70 >> batch$(($i+1))_$j.txt
	done
else
	echo $j >> batch$(($i+1))_$j.txt
	for k in `seq 1 10`
	do
	echo ../test/testloop$i 70 >> batch$(($i+1))_$j.txt
	done
fi
done
done