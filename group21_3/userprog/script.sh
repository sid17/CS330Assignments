cd ../userprog
#rm -rf data
#mkdir data
for i in 16 32 64 128 256 512; do	
	cd ../machine
	python modify.py $i
	cd ../userprog
	make clean
	make depend
	make
	mv nachos nachos$i
done

cd ../userprog
for i in queue vmtest1 vmtest2; do
	for j in 16 32 64 128 256 512; do
		for k in 1 2 3 4; do
		#echo "$i_$j_$k.txt";
		filename="data/"$i"_"$j"_"$k".txt";
		echo $filename;
		touch $filename;	
		#((time ./nachos$j -rs 0 -A 3 -P 100 -R $k -x ../test/$i) >  $filename 2>&1);
		`./nachos$j -rs 0 -A 3 -P 100 -R $k -x ../test/$i > $filename`;
		done
	done
done
