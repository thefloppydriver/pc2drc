#!/bin/bash

if [ `id -u` -ne 0 ]; then
    echo "Please run with: sudo -E ./EXPERIMENTAL-stage-1-kernel-patch.sh"
    exit
fi

if [ $(cd $HOME/.. && pwd) != "/home" ]; then
    echo "Please run with: sudo -E ./EXPERIMENTAL-stage-1-kernel-patch.sh"
    exit
fi


if [ ${PWD##*/} != "pc2drc" ]; then
  echo "Parent folder should be named pc2drc. If this script isn't in its intended directory there may be problems."
  read -n 1 -p "(press enter to continue)"
fi

if [[ $(ip link show | grep -o -m1 "\w*wl\w*") == "" ]]; then
    echo "No wireless interfaces found. Please attatch a wireless interface and try again."
    exit
fi

echo -e "GET http://google.com HTTP/1.0\n\n" | nc google.com 80 > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "An internet connection is required for this script to run. Try sudo service network-manager start."
    read -n 1 -p "(press enter to quit)"
    exit
fi


unload_modules_recursively () {
  local output=$(rmmod $@ 2>&1)
  if [[ $output =~ "by:" ]]; then
    unload_modules_recursively $(rmmod $output 2>&1 >/dev/null | grep -o -m1 "by: .*" | cut -c 5-)
  fi
  if [[ $output =~ "missing module name" ]]; then
    return 69 #this doesn't matter, I should really check this but whatever
  fi
  sleep 1
}


apt-get install git fakeroot build-essential ncurses-dev xz-utils libssl-dev bc flex libelf-dev bison dkms -y


#mkdir kernel-patch-files #todo, add gitignore linux-5.11.22.tar.xz and linux-5.11.22
cd kernel-patch-files 

kernel_version=$(uname -r | grep -Po -m1 "[0-9]*\.[0-9]*\.[0-9]+" | head -1)
kernel_version_first2parts=$(uname -r | grep -Po -m1 "[0-9]*\.[0-9]*" | head -1)
kernel_version_wget=$(uname -r | grep -Po -m1 "[0-9]*\.[0-9]*(\.[1-9]+)*" | head -1)
kernel_version_pt1=$(echo $kernel_version | grep -o -m1 "[0-9]*" | head -1)

if [[ -f "./linux-$kernel_version_wget.tar.xz" ]] || [[ -f "./linux-$kernel_version.tar.xz" ]]; then
    echo "Kernel already downloaded, skipping download."
else
    wget https://cdn.kernel.org/pub/linux/kernel/v${kernel_version_pt1}.x/linux-${kernel_version_wget}.tar.xz
fi

if [[ $kernel_version != $kernel_version_wget ]]; then
    test -d ./linux-${kernel_version_wget}.tar.xz && mv ./linux-${kernel_version_wget}.tar.xz ./linux-${kernel_version}.tar.xz
fi

if [[ -d "./linux-$kernel_version" ]]; then
    echo "Kernel already decompressed, skipping extraction."
else
    echo "Extracting kernel..."
    tar xvf linux-${kernel_version_wget}.tar.xz 2>&1 >/dev/null
    test -d ./linux-$kernel_version_wget && mv ./linux-${kernel_version_wget} ./linux-${kernel_version}
fi


cp -v /boot/config-$(uname -r) ./linux-${kernel_version}/.config


cd ./linux-${kernel_version}/net/mac80211

#exit

if [[ -f "./README.DRC" ]]; then
    echo "Kernel sources already patched, skipping patch."
else
    if [[ $(echo $kernel_version_first2parts'>5.11' | bc -l) == 1 ]]; then
        #XXX IMPORTANT REMINDER FOR FUTURE THEFLOPPYDRIVER: -p1 and -p2 strip leading dirnames from the patch, ./mac80211/iface.c -p2 = iface.c   mac80211/iface.c -p1 = iface.c
        patch -p2 < ../../../kernel_above_5_11_mac80211.patch
    else
        patch -p1 < ../../../kernel_below_5_12_mac80211.patch #add sudo if this doesn't work
    fi
fi

cd -

sed -i 's/[#]*CONFIG_SYSTEM_TRUSTED_KEYS/#CONFIG_SYSTEM_TRUSTED_KEYS/g' ./linux-${kernel_version}/.config #the [#]* is there to match any # characters just in case we already ran this script.


cd linux-${kernel_version}

echo
echo "This could take a few minutes depending on your system configuration"
read -n 1 -p "(press enter to build and install kernel patch)"

make olddefconfig -j`nproc` 

#cd .. 
##dir: pc2drc/kernel-patch-files

cd ./net

#cp -R ./linux-5.11.22/net/mac80211 ./mac80211

chown $USERNAME:$USERNAME ./mac80211/Makefile

sed -i 's/obj-$(CONFIG_MAC80211) += mac80211.o/obj-$(CONFIG_MAC80211) += mac80211.o\nKVERSION := $(shell uname -r)/g' ./mac80211/Makefile

if grep -xq "all:" ./mac80211/Makefile && grep -xq "clean:" ./mac80211/Makefile; then
    echo "Makefile already patched, skipping"
else
    echo -e "\nall:\n\t\$(MAKE) -C /lib/modules/\$(KVERSION)/build M=\$(PWD) modules\n\nclean:\n\t\$(MAKE) -C /lib/modules/\$(KVERSION)/build M=\$(PWD) clean" >> ./mac80211/Makefile
fi

if [ -f "./mac80211/dkms.conf" ]; then
    echo "dkms.conf exists, skipping"
else
    echo -e 'PACKAGE_NAME="drc-mac80211"\nPACKAGE_VERSION="0.1.0"\nCLEAN="make clean"\nMAKE[0]="make all KVERSION=$kernelver"\nBUILT_MODULE_NAME[0]="mac80211"\nDEST_MODULE_LOCATION[0]="/updates"\nAUTOINSTALL="yes"' > ./mac80211/dkms.conf
fi

if [ -d "/usr/src/drc-mac80211-0.1.0" ]; then
    rm -rf /usr/src/drc-mac80211-0.1.0
fi

cp -rf ./mac80211 /usr/src/drc-mac80211-0.1.0

cd ../

make modules_prepare -j`nproc`

dkms remove -m drc-mac80211/0.1.0 -k `uname -r`

dkms install -m drc-mac80211 -v 0.1.0 -j `nproc`





#ASSUME PATCHED MAC80211 MODULE IS IN /lib/modules/$(uname -r)/updates/dkms/mac80211.ko

restore_modules=$(rmmod mac80211 2>&1 >/dev/null | grep -o -m1 "by: .*" | cut -c 5-)

unload_modules_recursively mac80211

# v Unnecessary?
#sudo cp -f /lib/modules/$(uname -r)/updates/dkms/mac80211.ko /lib/modules/$(uname -r)/mac80211.ko

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


cd ../..


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
    echo 'Kernel patch not working. Try to run stage-1-kernel-patch.sh instead.'
    read -n 1 -p "(press enter to quit)"
    exit
fi
