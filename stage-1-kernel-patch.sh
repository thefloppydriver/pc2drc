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
SCRIPT_NAME="stage-1-kernel-patch.sh"
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
      read -n 1 -p "(press enter to continue)"
    fi
    
   #(DESC) check if there is a wifi interface
    if [[ $(ip link show | grep -o -m1 "\w*wl\w*") == "" ]]; then
        echo "No wireless interfaces found. Please attatch a wireless interface and try again."
        exit
    fi
    
   #(DESC) check for active internet connection
    echo -e "GET http://google.com HTTP/1.0\n\n" | nc google.com 80 > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "An internet connection is required for this script to run. Try \`sudo service network-manager start\`"
        read -n 1 -p "(press enter to quit)"
        exit
    fi
}; init #(DESC) start function immediately




#(DESC) Like rmmod but also unloads mod dependencies. `restore_modules=$(rmmod ${module_to_unload} 2>&1 >/dev/null | grep -o -m1 "by: .*" | cut -c 5-)` should be run before this and `modprobe $resore_modules` after.
unload_modules_recursively () {
  local output=$(rmmod $@ 2>&1)
  if [[ $output =~ "by:" ]]; then
    unload_modules_recursively $(rmmod $output 2>&1 >/dev/null | grep -o -m1 "by: .*" | cut -c 5-)
  fi
  if [[ $output =~ "missing module name" ]]; then
    return 69 #(TODO) this doesn't matter, I should really check this but whatever
  fi
  sleep 1
}


#(DESC) install required packages
apt-get install git fakeroot build-essential ncurses-dev xz-utils libssl-dev bc flex libelf-dev bison dkms -y

KERNEL_VER=`uname -r`

#(DESC) Everyone wants the kernel in a different format, create those formats here.
kernel_version_cdn=$(echo $KERNEL_VER | grep -Po -m1 "[0-9]*\.[0-9]*(\.[1-9]+)*" | head -1) #(DESC) cdn.kernel.org formats the kernel versions like so:  x.y.0 -> x.y   x.y.>0 -> x.y.z
kernel_version_x=$(echo $KERNEL_VER | grep -o -m1 "[0-9]*" | head -1) #(DESC) x.y.z -> x
kernel_version_y=$(echo $KERNEL_VER | grep -o -m1 "[0-9]*" | head -2 | tail -1) #(DESC) x.y.z -> y
kernel_version_z=$(echo $KERNEL_VER | grep -o -m1 "[0-9]*" | head -3 | tail -1) #(DESC) x.y.z -> z





cd ./kernel-patch-files



#(DESC) checks in case script has already been run
if [[ -f "./linux-${kernel_version_cdn}.tar.xz" ]] || [[ -f "./linux-${KERNEL_VER}.tar.xz" ]]; then
    echo "Kernel already downloaded, skipping download."
else
    wget https://cdn.kernel.org/pub/linux/kernel/v${kernel_version_x}.x/linux-${kernel_version_cdn}.tar.xz
fi


#(DESC) if kernel hasn't been extracted already extract kernel and rename to PATCH_KERNEL_VER if KERNEL_VER != kernel_version_cdn
if [[ -d "./linux-$KERNEL_VER" ]]; then
    echo "Kernel already decompressed, skipping extraction."
elif [[ -d "./linux-$kernel_version_cdn" ]]; then
    echo "Kernel already decompressed, not renamed. Skipping extraction and renaming."
    test -d ./linux-$kernel_version_cdn && mv ./linux-${kernel_version_cdn} ./linux-${KERNEL_VER}
else
    echo "Extracting kernel..."
    tar xvf linux-${kernel_version_cdn}.tar.xz 2>&1 >/dev/null
    if [[ $kernel_version_cdn != $KERNEL_VER ]]; then
        test -d ./linux-${kernel_version_cdn} && mv ./linux-${kernel_version_cdn} ./linux-${KERNEL_VER}
    fi
fi

cp -v /boot/config-$(uname -r) ./linux-${KERNEL_VER}/.config


if [[ -f "./linux-${KERNEL_VER}/net/mac80211/README.DRC" ]]; then
    echo "Kernel sources already patched, skipping patch."
else
    if [[ $(echo "${kernel_version_x}.${kernel_version_y}>5.11" | bc -l) == 1 ]]; then
        #(NOTE) -p1 and -p2 strip leading dirnames from the patch, ./mac80211/iface.c -p2 = iface.c   mac80211/iface.c -p1 = iface.c
        #(CODE) patch -p2 < ../../../kernel_above_5_11_mac80211.patch
        patch -p2 -d "./linux-${KERNEL_VER}/net/mac80211" -i "../../../kernel_above_5_11_mac80211.patch"
    else
        #(CODE) patch -p1 < ../../../mac80211.patch #add sudo if this doesn't work
        patch -p1 -d "./linux-${KERNEL_VER}/net/mac80211" -i "../../../kernel_below_5_12_mac80211.patch"
    fi
fi



sed -i 's/[#]*CONFIG_SYSTEM_TRUSTED_KEYS/#CONFIG_SYSTEM_TRUSTED_KEYS/g' "./linux-${KERNEL_VER}/.config" #(NOTE) the [#]* is there to match any # characters just in case we already ran this script.


#cd linux-${kernel_version}

echo
echo "This could take a few minutes depending on your system configuration"
read -n 1 -p "(press enter to build and install kernel patch)"

#(DESC) Configure patched linux kernel sources
make -C ./linux-${KERNEL_VER} olddefconfig -j`nproc` 


mac80211_dir=./linux-${KERNEL_VER}/net/mac80211


chown $USERNAME:$USERNAME $mac80211_dir/Makefile

sed -i 's/obj-$(CONFIG_MAC80211) += mac80211.o/obj-$(CONFIG_MAC80211) += mac80211.o\nKVERSION := $(shell uname -r)/g' $mac80211_dir/Makefile

if grep -xq "all:" $mac80211_dir/Makefile && grep -xq "clean:" $mac80211_dir/Makefile; then
    echo "Makefile already patched, skipping"
else
    echo -e "\nall:\n\t\$(MAKE) -C /lib/modules/\$(KVERSION)/build M=\$(PWD) modules\n\nclean:\n\t\$(MAKE) -C /lib/modules/\$(KVERSION)/build M=\$(PWD) clean" >> $mac80211_dir/Makefile
fi

if [ -f "${mac80211_dir}/mac80211/dkms.conf" ]; then
    echo "dkms.conf exists, skipping"
else
    echo -e 'PACKAGE_NAME="drc-mac80211"\nPACKAGE_VERSION="0.1.0"\nCLEAN="make clean"\nMAKE[0]="make all KVERSION=$kernelver"\nBUILT_MODULE_NAME[0]="mac80211"\nDEST_MODULE_LOCATION[0]="/updates"\nAUTOINSTALL="yes"' > $mac80211_dir/dkms.conf
fi

if [ -d "/usr/src/drc-mac80211-0.1.0" ]; then
    rm -rf /usr/src/drc-mac80211-0.1.0
fi

cp -rf $mac80211_dir /usr/src/drc-mac80211-0.1.0



make -C ./linux-${KERNEL_VER} modules_prepare -j`nproc`

dkms remove -m drc-mac80211/0.1.0 -k `uname -r`

dkms install -m drc-mac80211 -v 0.1.0 -j `nproc`


cd ..




#(NOTE) ASSUME PATCHED MAC80211 MODULE IS IN /lib/modules/$(uname -r)/updates/dkms/mac80211.ko

restore_modules=$(rmmod mac80211 2>&1 >/dev/null | grep -o -m1 "by: .*" | cut -c 5-)

unload_modules_recursively mac80211



insmod /lib/modules/$(uname -r)/updates/dkms/mac80211.ko 2>&1 | grep -v "File exists"
if [[ $(awk '{ print $1 }' /proc/modules | xargs modinfo -n | grep "mac80211") =~ "dkms/mac80211.ko" ]]; then
    echo "Patched mac80211 module installed"
else
    insmod /lib/modules/$(uname -r)/updates/dkms/mac80211.ko 2>&1 | grep -v "File exists"
    if [[ $(awk '{ print $1 }' /proc/modules | xargs modinfo -n | grep "mac80211") =~ "dkms/mac80211.ko" ]]; then
        echo "Patched mac80211 module installed"
    else        
        debug_file=./generated_bug_report.txt
        echo "SCRIPT NAME: ${0}" > $debug_file
        echo "START OF COMMAND awk '{ print $1 }' /proc/modules | xargs modinfo -n" >> $debug_file
        awk '{ print $1 }' /proc/modules | xargs modinfo -n >> $debug_file
        echo -e "\n\n\n\n\nSTART OF COMMAND modinfo iwlmvm | sed -n '/sig_id/q;p'" >> $debug_file
        modinfo mac80211 | sed -n '/sig_id/q;p' >> $debug_file
        
        echo        
        echo "FATAL ERROR: Could not load patched mac80211 module."
        echo "thefloppydriver: I have no idea what just caused this to happen. Please send a detailed bug report if you get this message so that I can catch it properly!!"
        echo "also attatch $(pwd)/generated_bug_report.txt to your bug report :)"
        echo
        read -n 1 -p "(press enter to quit)"
        modprobe $restore_modules
        exit
    fi
fi

echo $restore_modules

#modprobe mac80211

modprobe $restore_modules

#rmmod $restore_modules && rmmod mac80211 && modprobe mac80211 && modprobe $restore_modules

unload_modules_recursively $restore_modules
unload_modules_recursively mac80211
modprobe mac80211
modprobe $restore_modules





echo 
echo 
echo 
echo 


if [[ -f "/sys/class/net/$(ip link show | grep -o -m1 "\w*wl\w*")/tsf" ]]; then
    rm -rf ./kernel-patch-files/linux-*
    echo 'Patched mac80211 loaded'
    echo "Kernel patch installed successfully"
    echo "Make sure to run stage 2 next!"
    read -n 1 -p "(press enter to quit)"
    exit
else
    echo 'Kernel patch not working. Try rebooting and running this script again. If that fails, run `sudo -E ./kernel-patch-files/build-patched-kernel.sh` instead.'
    read -n 1 -p "(press enter to quit)"
    exit
fi
