#!/bin/bash

if [ -z $1 ]
then
    echo "Need to specify a platform (ex.: vexpress, rpi4, virt-riscv64, etc.)"
    exit
fi

mkimage -f $1.its $1.itb 


