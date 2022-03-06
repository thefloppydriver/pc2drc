#!/bin/bash

#key:
 #(<type>) <code or comment>
 #
 #(DESC)       Describes what a function does
 #(NOTE)       Note about a particular bit of code
 #(TODO)       Future implementation
 #(BAD)        This code is bad. Replace ASAP.
 #(UGLY)       This code is unreadable. Replace ASAP.
 #(WRKARND)    This is a workaround to some problem. Best not to touch.
 #(EXCEPT)     Error checking code
 #(UNTESTED)   Untested code
 #(CODE)       Code that's been commented out or obseleted
 #(DEBUG)      Uncomment for a verified debugging moment
 #(FIX)        Fixes a github issue
 #(SANITY)     This line has or is a sanity check
 #(CLEANUP)    This code cleans up a mess made by some previous code
 #(XXX)        Bookmark or where the dev left off aka LOOK AT ME!!


#(DESC) Template constants
SCRIPT_NAME="stage-0-init-script.sh"
SCRIPT_PARENT_DIR_NAME="pc2drc"
SCRIPT_DIR=$(pwd)



init () {
   #(DESC) check if user is root
    if [ `id -u` -ne 0 ]; then
        echo "Please run with: sudo -E ./${SCRIPT_NAME}"
        exit
    fi
    
   #(DESC) check if non-root user's environment variables have been passed through
    if [ $(cd $HOME/.. && pwd) != "/home" ]; then
        echo "Please run with: sudo -E ./${SCRIPT_NAME}"
        exit
    fi
    
   #(DESC) check if script is in intended directory
    if [ ${PWD##*/} != $SCRIPT_PARENT_DIR_NAME ]; then
      echo "Parent folder should be named ${SCRIPT_PARENT_DIR_NAME}. If this script isn't in its intended directory there may be problems."
      read -p "(press enter to continue)"
    fi
    
   #(DESC) check if there is a wifi interface
    if [[ $(ip link show | grep -o -m1 "\w*wl\w*") == "" ]]; then
        echo "No wireless interfaces found. Please attatch a wireless interface and try again."
        read -p "(press enter to quit)"
        exit
    fi
    
   #(DESC) check for active internet connection
    echo -e "GET http://google.com HTTP/1.0\n\n" | nc google.com 80 > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "An internet connection is required for the following two scripts to run. Try \`sudo service network-manager start\`"
        read -p "(press enter to quit)"
        exit
    fi
}; init #(DESC) start function immediately



chmod 775 ./kernel-patch-files/build-patched-kernel.sh
chmod 775 ./stage-1-kernel-patch.sh
chmod 775 ./stage-2-build-modules.sh
chmod 775 ./stage-3-pair-with-wiiu.py
chmod 775 ./stage-4-start-pc2drc.py
chmod 775 ./start-pc2drc.sh

echo "Good to go!"
echo
echo "To start a script, type:     sudo -E ./bash-script-name.sh     OR     sudo -E python3 ./python-script-name.py"
