#!/bin/bash
#count the line of code

lineNumeber=0

for fileName in *
do
  suffix=${fileName##*.}

  if [ $suffix = "h" -o $suffix = "c" -o $suffix = "cpp" ]
    then
      line=$(wc -l $fileName | awk '{print $1}')
      let lineNumber+=line
  fi
done

echo "Total line number : $lineNumber"

exit 0
