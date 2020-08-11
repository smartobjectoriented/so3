#!/bin/bash

other_qemu=`sudo brctl show | grep tap | wc -l`
if [[ ${other_qemu} -eq 1 ]]
then
    sudo ifconfig br0 down
    sudo brctl delbr br0
    sudo killall dnsmasq
    #sudo kill `ps aux | grep "dnsmasq --strict-order" | grep -v "grep" | awk "{ print \\$2; }"`
fi

