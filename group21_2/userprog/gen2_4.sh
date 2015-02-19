
echo '\section{Batch 6} 
 \begin{adjustwidth}{-2.8cm}{} 
\begin{tabular}{|l|l|l|l|l|l|l|} 
\hline 
Algorithm Type & Scheduling Policy &  Maximum & Minimum & Average & Variance of job completion 
times  \\ 
\hline ' >> result_1.txt


  echo `./nachos -F c1.txt` 
  echo `./nachos -F c2.txt`

echo '\end{tabular}
\end{adjustwidth}
' >> result_1.txt

