#!/usr/bin/gnuplot -persist

#set title "Progression"
set xlabel "Hours"
set ylabel "Temp"
set grid


set terminal jpeg size 1024,768 font Helvetica 20 enhance
set output "LastPlot.jpg"

plot "LastDataFile.csv" u ($1/3600):2 smooth bezier notitle
