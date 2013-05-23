#!/bin/bash

if [ -e "file" ] 
  then
    echo "file is exist"
    (cd "file" ; \rm * 2>/dev/null)
fi

if [ -e "request" ] 
  then
    echo "request is exist"
    (cd "request" ; \rm * 2>/dev/null)
fi

if [ -e "printer" ] 
  then
    echo "printer is exist"
    (cd "printer" ; \rm * 2>/dev/null)
fi
