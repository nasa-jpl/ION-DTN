#!/bin/bash

ps a | grep ecls | grep -v  grep | sed 's/^ *//g' | cut -f1 -d' '| while read line
do
kill -9 $line
done
killm 

echo "ion and eclsa killed"
exit 0
