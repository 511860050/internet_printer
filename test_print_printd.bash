#!/bin/bash

./clear_all.bash

for file in $(ls print_file)
do
  (./client print_file/$file)&
done

sleep 1

#check the number of create file 
number=$(ls print_file | awk '{count++}END{print count}')
fileNumber=$(ls file | awk '{count++}END{print count}')
requestNumber=$(ls request | awk '{count++}END{print count}')

echo "source files's number : $number"
echo "copy files's number : $fileNumber"
echo "request files's number : $requestNumber"

if [ $fileNumber -ne $number -o $requestNumber -ne $number ]
  then
    echo "Failure in number"
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
        echo "Failure in contains"
        exit -1
    fi
   done
)

echo "success"
exit 0
