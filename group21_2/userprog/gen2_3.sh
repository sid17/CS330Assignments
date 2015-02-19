
echo '\section{Batch 5} 
 \begin{adjustwidth}{0cm}{} 
\begin{tabular}{|l|l|l|} 
\hline 
Algorithm Type & Scheduling Policy &  Average Estimation Error \\ 
\hline ' >> result_1.txt


  echo `./nachos -F batch1_2.txt` 
  echo `./nachos -F batch2_2.txt` 
  echo `./nachos -F batch3_2.txt` 
  echo `./nachos -F batch4_2.txt` 
  echo `./nachos -F t2.txt` 

echo '\end{tabular}
\end{adjustwidth}
' >> result_1.txt

