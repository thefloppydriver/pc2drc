#!/bin/bash

if [ `id -u` -ne 0 ]; then
    echo "Please run with: sudo -E ./start-pc2drc.sh"
    exit
fi

if [ $(cd $HOME/.. && pwd) != "/home" ]; then
    echo "Please run with: sudo -E ./start-pc2drc.sh"
    exit
fi

if [ ! -f "/sys/class/net/$(ip link show | grep -o -m1 "\w*wl\w*")/tsf" ]; then
   echo 'TSF kernel patch not loaded.'
   if [ $(uname -r) != '5.11.22' ] ; then
       echo "You're running an unpatched kernel."
       echo "Please reboot to grub and choose Advanced options for Ubuntu > Ubuntu, with Linux 5.11.22   (It should be the selected by default)"
       read -n 1 -p "(press enter to quit)"
       exit
   fi
   echo "You're running the patched kernel version, but the patch isn't working."
   echo "Please reboot and try again or email thefloppydriver@gmail.com for help."
   exit
fi

script_dir=$(pwd)

python3 ./stage-4-start-pc2drc.py
