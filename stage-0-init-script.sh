#!/bin/bash

if [ `id -u` -ne 0 ]; then
    echo "Please run with: sudo -E ./stage-0-init-script.sh"
    exit
fi

if [ $(cd $HOME/.. && pwd) != "/home" ]; then
    echo "Please run with: sudo -E ./stage-0-init-script.sh"
    exit
fi

echo -e "GET http://google.com HTTP/1.0\n\n" | nc google.com 80 > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "An internet connection is required for the following two scripts to run. Try sudo service network-manager start."
    read -n 1 -p "(press enter to quit)"
    exit
fi

chmod 775 ./stage-1-kernel-patch.sh
chmod 775 ./stage-2-build-modules.sh
chmod 775 ./stage-3-pair-with-wiiu.py
chmod 775 ./stage-4-start-pc2drc.py
chmod 775 ./start-pc2drc.sh

echo "Good to go!"
echo 'To start a script, type sudo -E ./bash-script-name.sh     OR     sudo -E python3 ./python-script-name.py'
