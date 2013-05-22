#!/bin/bash
#============================================
#FileName: test_whole.bash
#Date: 2013年 05月 22日 星期三 21:04:22 CST
#Author: chenhuan
#Usage: 
#============================================

./clear_all.bash

for file in $(ls print_file)
do
  (./client print_file/$file)&
done

sleep 1
