#!/bin/bash

chmod 775 ./stage-1-kernel-patch.sh
chmod 775 ./stage-2-build-modules.sh
chmod 775 ./stage-3-pair-with-wiiu.py
chmod 775 ./stage-4-start-pc2drc.py

echo "Good to go!"
echo 'To start a script, type sudo -E ./bash-script-name.sh     OR     sudo -E python3 ./python-script-name.py'
