#!/usr/bin/gnuplot

set term png
set output "graph2_Execution.png"
set title "Total Execution Time vs Quantum"
set xlabel "Time Quantum"
set ylabel "Total Execution Time"
plot 'result_1.txt' using 1:2 with lines