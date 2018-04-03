#!/bin/bash

SAMPLEINTERVAL=30
LINKNAME=LastDataFile.csv
LOGFILE=Altec-$(date --iso-8601=seconds).csv
RUNTIME=0

touch $LOGFILE
rm -f $LINKNAME
ln -s $LOGFILE $LINKNAME

while true
do

echo $RUNTIME   $(./sendaltec -n /dev/ttyUSB0 01 PV) >> $LOGFILE

sleep $SAMPLEINTERVAL
RUNTIME=$((RUNTIME+SAMPLEINTERVAL))

done

