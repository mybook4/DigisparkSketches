#!/bin/bash

# check that there was the right number of command line arguments
if [ "$#" -ne 1 ]; then
  echo "Usage: "$0" <input set file>"
  exit
fi

# replace all periods in the file name with underscores
name=`echo $1 | sed 's/\./_/g'`

# start creating an array from the set file's name
echo "const uint8_t programArray"$name"[] PROGMEM = {" > $name.h

# print first line in the file
echo -n `head -n 1 $1` >> $name.h

sed 1d $1 | while read p; do
  echo "," >> $name.h
  echo -n $p >> $name.h  
done

echo "" >> $name.h
echo "};" >> $name.h


