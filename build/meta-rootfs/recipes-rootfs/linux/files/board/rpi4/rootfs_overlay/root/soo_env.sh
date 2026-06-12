
# This script is sourced by initd script

export WIFI_SSID="soo-domotics"
export WIFI_CHANNEL=40
export WIFI_BANDWIDTH=""
export SOO_NAME="SOO-rpi4"
export BT_NAME="SOO-rpi4-BT"

echo ${SOO_NAME} > /sys/soo/name
echo ${BT_NAME} > /etc/hostname

