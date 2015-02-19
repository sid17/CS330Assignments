
echo '\section{Batch 5} 
 \begin{adjustwidth}{-2.8cm}{} 
\begin{tabular}{|l|l|l|} 
\hline 
Algorithm Type & Scheduling Policy &  Average waiting time \\ 
\hline ' >> result_1.txt


   echo `./nachos -F t1.txt` 

  echo `./nachos -F t2.txt` 

echo '\end{tabular}
\end{adjustwidth}
' >> result_1.txt

