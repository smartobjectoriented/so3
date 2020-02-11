#!/bin/bash
#
# * -- Smart Object Oriented  --
# * Copyright (c) 2016,2017 Sootech SA, Switzerland
# * 
# * The contents of this file is strictly under the property of Sootech SA and must not be shared in any case.
# *
# * Contributors:
# *
# * - October 2017: Baptiste Delporte
#

other_qemu=`sudo brctl show | grep tap | wc -l`
if [[ ${other_qemu} -eq 1 ]]
then
    sudo ifconfig br0 down
    sudo brctl delbr br0
    sudo killall dnsmasq
    #sudo kill `ps aux | grep "dnsmasq --strict-order" | grep -v "grep" | awk "{ print \\$2; }"`
fi

