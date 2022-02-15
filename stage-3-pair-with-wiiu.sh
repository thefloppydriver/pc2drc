#!/bin/bash

if [ `id -u` -ne 0 ]; then
    echo "Please run with: sudo -E ./stage-3-pair-with-wiiu.sh"
    exit
fi

if [ $(cd $HOME/.. && pwd) != "/home" ]; then
    echo "Please run with: sudo -E ./stage-3-pair-with-wiiu.sh"
    exit
fi



WLAN=$(ip link show | grep -o -m1 "\w*: wl\w*" | grep -o "\w*wl\w*")
ip link set dev $WLAN up
#sudo iw $WLAN scan | grep -A9 "\w*BSS \S\S:\S\S:\S\S:\S\S:\S\S:\S\S\w*" | grep -o "\S\S:\S\S:\S\S:\S\S:\S\S:\S\S
#SSID: \w*" | grep -B1 "WiiU\S\S\S\S\S\S\S\S\S\S\S\S"

service network-manager stop

cp -f ./drc-hostap/conf/get_psk.conf.orig ./drc-hostap/wpa_supplicant/get_psk.conf

chown -R $USERNAME:$USERNAME ./drc-hostap/wpa_supplicant/get_psk.conf

./drc-hostap/wpa_supplicant/wpa_supplicant -Dnl80211 -i $WLAN -c ./drc-hostap/wpa_supplicant/get_psk.conf


