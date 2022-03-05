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
SCRIPT_NAME="build-patched-kernel.sh"
SCRIPT_PARENT_DIR_NAME="kernel-patch-files"
SCRIPT_DIR=$(pwd)


#(DESC) Other constants
PATCH_KERNEL_VER="5.11.22" #(DESC) Known supported kernel to be built, patched, and installed.



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
    
   #(DESC) check for active internet connection
    echo -e "GET http://google.com HTTP/1.0\n\n" | nc google.com 80 > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "An internet connection is required for this script to run. Try \`sudo service network-manager start\`"
        read -n 1 -p "(press enter to quit)"
        exit
    fi
}; init #(DESC) start function immediately


#(DESC) install required packages
apt-get install git fakeroot build-essential ncurses-dev xz-utils libssl-dev bc flex libelf-dev bison -y


#(DESC) Everyone wants the kernel in a different format, create those formats here.
kernel_version_cdn=$(echo $PATCH_KERNEL_VER | grep -Po -m1 "[0-9]*\.[0-9]*(\.[1-9]+)*" | head -1) #(DESC) cdn.kernel.org formats the kernel versions like so:  x.y.0 -> x.y   x.y.>0 -> x.y.z
kernel_version_x=$(echo $PATCH_KERNEL_VER | grep -o -m1 "[0-9]*" | head -1) #(DESC) x.y.z -> x
kernel_version_y=$(echo $PATCH_KERNEL_VER | grep -o -m1 "[0-9]*" | head -2 | tail -1) #(DESC) x.y.z -> y
kernel_version_z=$(echo $PATCH_KERNEL_VER | grep -o -m1 "[0-9]*" | head -3 | tail -1) #(DESC) x.y.z -> z


#(DESC) checks in case script has already been run
if [[ -f "./linux-${kernel_version_cdn}.tar.xz" ]] || [[ -f "./linux-${PATCH_KERNEL_VER}.tar.xz" ]]; then
    echo "Kernel already downloaded, skipping download."
else
    wget https://cdn.kernel.org/pub/linux/kernel/v${kernel_version_x}.x/linux-${kernel_version_cdn}.tar.xz
fi

#(DESC) if kernel hasn't been extracted already extract kernel and rename to PATCH_KERNEL_VER if PATCH_KERNEL_VER != kernel_version_cdn
if [[ -d "./linux-$PATCH_KERNEL_VER" ]]; then
    echo "Kernel already decompressed, skipping extraction."
elif [[ -d "./linux-$kernel_version_cdn" ]]; then
    echo "Kernel already decompressed, not renamed. Skipping extraction and renaming."
    test -d ./linux-$kernel_version_cdn && mv ./linux-${kernel_version_cdn} ./linux-${PATCH_KERNEL_VER}
else
    echo "Extracting kernel..."
    tar xvf linux-${kernel_version_cdn}.tar.xz 2>&1 >/dev/null
    if [[ $kernel_version_cdn != $PATCH_KERNEL_VER ]]; then
        test -d ./linux-${kernel_version_cdn} && mv ./linux-${kernel_version_cdn} ./linux-${PATCH_KERNEL_VER}
    fi
fi

cp -v /boot/config-$(uname -r) ./linux-${PATCH_KERNEL_VER}/.config


#(CODE) cd ./linux-5.11.22/net/mac80211

if [[ -f "./linux-${PATCH_KERNEL_VER}/net/mac80211/README.DRC" ]]; then
    echo "Kernel sources already patched, skipping patch."
else
    if [[ $(echo "${kernel_version_x}.${kernel_version_y}>5.11" | bc -l) == 1 ]]; then
        #(NOTE) -p1 and -p2 strip leading dirnames from the patch, ./mac80211/iface.c -p2 = iface.c   mac80211/iface.c -p1 = iface.c
        #(CODE) patch -p2 < ../../../kernel_above_5_11_mac80211.patch
        patch -p2 -d "./linux-${PATCH_KERNEL_VER}/net/mac80211" -i "../../../kernel_above_5_11_mac80211.patch"
    else
        #(CODE) patch -p1 < ../../../mac80211.patch #add sudo if this doesn't work
        patch -p1 -d "./linux-${PATCH_KERNEL_VER}/net/mac80211" -i "../../../kernel_below_5_12_mac80211.patch"
    fi
fi


#(CODE) cd -


sed -i 's/[#]*CONFIG_SYSTEM_TRUSTED_KEYS/#CONFIG_SYSTEM_TRUSTED_KEYS/g' "./linux-${PATCH_KERNEL_VER}/.config" #(NOTE) the [#]* is there to match any # characters just in case we already ran this script.


#(CODE) cd linux-5.11.22


echo "This could take over 2 hours depending on your system configuration"
read -n 1 -p "(press enter to recompile the linux kernel)"

#(DESC) Configure, compile, and install the patched linux kernel
make -C ./linux-${PATCH_KERNEL_VER} olddefconfig -j`nproc`

#(NOTE) this *will* take a while. 
make -C ./linux-${PATCH_KERNEL_VER} -j`nproc` #(FIX) Issue #1

make -C ./linux-${PATCH_KERNEL_VER} modules_install -j`nproc`

make -C ./linux-${PATCH_KERNEL_VER} install -j`nproc`



#(DESC) Edit grub to make patched kernel version default
cp /etc/default/grub /etc/default/grub.backupfile

sed -i -e "s/GRUB_DEFAULT\\=0/GRUB_DEFAULT\\=\"Advanced\\ options\\ for\\ Ubuntu\\>Ubuntu,\\ with Linux\\ ${PATCH_KERNEL_VER}\"/" /etc/default/grub #(UGLY)

update-grub

cp /boot/grub/grub.cfg /boot/grub/grub.cfg.backupfile

line_index_1=$(grep -wn -m1 "menuentry 'Ubuntu' --class ubuntu.*" /boot/grub/grub.cfg | cut -d: -f1)
line_index_2_rel_index_1=$(tail -n +$line_index_1 /boot/grub/grub.cfg | grep -wn -m1 "}" | cut -d: -f1)
line_index_2=$((line_index_1 + line_index_2_rel_index_1))

string_to_be_parsed=$(tail -n +$line_index_1 /boot/grub/grub.cfg | head -n +$line_index_2_rel_index_1)

replace_line_index_1_rel=$(echo "$string_to_be_parsed" | grep -wn -m1 $'\t'"linux"$'\t'"/boot/\S*" | cut -d: -f1)
replace_line_index_2_rel=$(echo "$string_to_be_parsed" | grep -wn -m1 $'\t'"initrd"$'\t'"/boot/.*" | cut -d: -f1)

replace_line_index_1=$((replace_line_index_1_rel + line_index_1 - 1))
replace_line_index_2=$((replace_line_index_2_rel + line_index_1 - 1))


sed -i "${replace_line_index_1}s?"$'\t'"linux"$'\t'"\S*?"$'\t'"linux"$'\t'"/boot/vmlinuz-${kernel_version_x}\.${kernel_version_y}\.${kernel_version_z}?" /boot/grub/grub.cfg #(UGLY)
sed -i "${replace_line_index_2}s?"$'\t'"initrd"$'\t'"\S*?"$'\t'"initrd"$'\t'"/boot/initrd\.img-${kernel_version_x}\.${kernel_version_y}\.${kernel_version_z}?" /boot/grub/grub.cfg #(UGLY)




#(DESC) Cleanup.
rm -rf ../kernel-patch-files/linux-* #(SANITY)

echo 
echo 
echo 
echo 
echo "Patched kernel installed successfully!"
echo "Make sure to run stage 2 after reboot!"
#(CODE) echo "And make sure to choose Advanced options for Ubuntu > Ubuntu, with Linux 5.11.22 in the grub menu! (or just don't touch anything for ~10 seconds)"
read -n 1 -p "(press enter to reboot)"

reboot
