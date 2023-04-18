#!/bin/bash

if [ -z $1 ]
then
    echo "Need to specify a platform (ex.: vexpress, virt64, rpi4, rpi4_64, etc.)"
    exit
fi

mkimage -f $1.its $1.itb

