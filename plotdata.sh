#!/bin/bash

INTERVAL=60
LINKNAME=LastDataFile.csv
PLOTDEF=graph.plt

TITLENAME=$(ls -la $LINKNAME | cut -d ' ' -f 11)


while true
do

LASTVALUE=$(tail -n1 $LINKNAME | cut -d ' ' -f 2)
gnuplot -e "set title \"$TITLENAME\"; set label \"Last Value=$LASTVALUE\" at 1,20" $PLOTDEF
cp LastPlot.jpg /var/www/localhost/htdocs

sleep $INTERVAL

done

