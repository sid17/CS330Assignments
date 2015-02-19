for j in `seq 1 4`
do

echo '\section{Batch `$j`} 
 \begin{adjustwidth}{-2.8cm}{} 
\begin{tabular}{|l|l|l|l|} 
\hline 
Algorithm Type & Scheduling Policy & CPU Utilization & Average waiting time \\ 
\hline ' >> result_1.txt

for k in `seq 1 10`
do
   echo `./nachos -F batch$j\_$k.txt` 
done

echo '\end{tabular}
\end{adjustwidth}
' >> result_1.txt


done