#!/bin/bash

./clear_all.bash

for file in $(ls print_file)
do
  (./client print_file/$file)&
done

exit 0
