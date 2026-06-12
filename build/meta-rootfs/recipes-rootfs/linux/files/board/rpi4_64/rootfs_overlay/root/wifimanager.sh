#!/bin/sh
iw reg set CH
iw wlan0 set type ibss
iwconfig wlan0 essid "soo-wifi"
ifconfig wlan0 up
iw wlan0 ibss join soo-wifi 5180

