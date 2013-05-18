#!/bin/bash

./clear_all.bash

for file in $(ls print_file)
do
  (./client print_file/$file)&
done

#check the number of create file 
number=$(ls print_file | awk '{count++}END{print count}')
fileNumber=$(ls file | awk '{count++}END{print count}')
requestNumber=$(ls request | awk '{count++}END{print count}')

if [ $fileNumber -ne $number -o $requestNumber -ne $number ]
  then
    echo "failure in number"
    exit -1
fi

#check the contains of create file
pointedFile="../print_file/1.txt"
(
  cd file
  for fileName in $(ls)
  do
    diff $fileName $pointedFile
    if [ $? -ne 0 ]
      then
        echo "failure in contains"
        exit -1
    fi
   done
)

echo "success"
exit 0
