#!/bin/bash

# BT_TTY="/dev/ttyAMA0"


# The MAC address is generated using some agency UID bytes
AGENCYUID_FILE="/sys/soo/agencyUID"
AGENCYUID=`cat ${AGENCYUID_FILE}`
BDADDR=${AGENCYUID:0:2}:${AGENCYUID:2:2}:${AGENCYUID:4:2}:${AGENCYUID:6:2}:${AGENCYUID:8:2}:${AGENCYUID:10:2}
echo "Bluetooth configuration..."
echo "- with MAC:" ${AGENCYUID}

# Configure the controller with an address and load its firmware
# hciattach ${BT_TTY} bcm43xx 460800 flow - ${BDADDR}

DEFAULT_HCI_NAME="soo-bt"

if [[ -n ${BT_NAME} ]]
then
    HCI_NAME=${BT_NAME}
else
    HCI_NAME=${DEFAULT_HCI_NAME}
fi

killall -9 /usr/libexec/bluetooth/bluetoothd
# Launch the BT daemon. The --compat is MANDATORY
/usr/libexec/bluetooth/bluetoothd --compat &
hciconfig hci0 up
# Secure Simple Pairing Mode
hciconfig hci0 sspmode 1
# Discoverable
hciconfig hci0 piscan
# Class Networking TTY interface
hciconfig hci0 class 001101
# Assign a name to the controller
hciconfig hci0 name ${HCI_NAME}
# Serial Port config
sdptool add --channel=1 SP
# Launch the agent with NoInputNoOutput so we don't have to enter a PIN 
bt-agent -c NoInputNoOutput -d

echo "Bluetooth configutation DONE!"

# This opens a rfcomm socket which will be hooked by vuihandler
while true
do
    # Kill RFCOMM instance if any
    killall -9 rfcomm > /dev/null 2>&1
    rfcomm release all
    # Watch for RFCOMM connections
    rfcomm -r watch 1
done




