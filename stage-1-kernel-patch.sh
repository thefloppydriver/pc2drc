#!/bin/bash

if [ `id -u` -ne 0 ]; then
    echo "Please run with: sudo -E ./stage-1-kernel-patch.sh"
    exit
fi

if [ $(cd $HOME/.. && pwd) != "/home" ]; then
    echo "Please run with: sudo -E ./stage-1-kernel-patch.sh"
    exit
fi


if [ ${PWD##*/} != "pc2drc" ]; then
  echo "Parent folder should be named pc2drc. If this script isn't in its intended directory there may be problems."
  read -n 1 -p "(press enter to continue)"
fi

echo -e "GET http://google.com HTTP/1.0\n\n" | nc google.com 80 > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "An internet connection is required for this script to run. Try sudo service network-manager start."
    read -n 1 -p "(press enter to quit)"
    exit
fi

apt-get install git fakeroot build-essential ncurses-dev xz-utils libssl-dev bc flex libelf-dev bison -y

#mkdir kernel-patch-files #todo, add gitignore linux-5.11.22.tar.xz and linux-5.11.22
cd kernel-patch-files 

if [[ -f "./linux-5.11.22.tar.xz" ]]; then
    echo "Kernel already downloaded, skipping download."
else
    wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.11.22.tar.xz
fi

if [[ -d "./linux-5.11.22" ]]; then
    echo "Kernel already decompressed, skipping extraction."
else
    tar xvf linux-5.11.22.tar.xz
fi

cp -v /boot/config-$(uname -r) ./linux-5.11.22/.config


cd ./linux-5.11.22/net/mac80211

if [[ -f "./README.DRC" ]]; then
    echo "Kernel sources already patched, skipping patch."
else
    patch -p1 < ../../../mac80211.patch #add sudo if this doesn't work
fi

cd -


sed -i 's/[#]*CONFIG_SYSTEM_TRUSTED_KEYS/#CONFIG_SYSTEM_TRUSTED_KEYS/g' ./linux-5.11.22/.config #the [#]* is there to match any # characters just in case we already ran this script.


cd linux-5.11.22

echo "This could take 1 to 2 hours depending on your system configuration"
read -n 1 -p "(press enter to recompile the linux kernel)"

make olddefconfig -j`nproc` #this *will* take a while. 

make -j`nproc` #Issue #1

make modules_install -j`nproc`

make install -j`nproc`

cp /etc/default/grub /etc/default/grub.backupfilebaybee

sed -i -e 's/GRUB_DEFAULT\=0/GRUB_DEFAULT\="Advanced\ options\ for\ Ubuntu\>Ubuntu,\ with Linux\ 5.11.22"/' /etc/default/grub

update-grub

cp /boot/grub/grub.cfg /boot/grub/grub.cfg.backupfilebaybee

line_index_1=$(grep -wn -m1 "menuentry 'Ubuntu' --class ubuntu.*" /boot/grub/grub.cfg | cut -d: -f1)
line_index_2_rel_index_1=$(tail -n +$line_index_1 /boot/grub/grub.cfg | grep -wn -m1 "}" | cut -d: -f1)
line_index_2=$((line_index_1 + line_index_2_rel_index_1))

string_to_be_parsed=$(tail -n +$line_index_1 /boot/grub/grub.cfg | head -n +$line_index_2_rel_index_1)

replace_line_index_1_rel=$(echo "$string_to_be_parsed" | grep -wn -m1 $'\t'"linux"$'\t'"/boot/\S*" | cut -d: -f1)
replace_line_index_2_rel=$(echo "$string_to_be_parsed" | grep -wn -m1 $'\t'"initrd"$'\t'"/boot/.*" | cut -d: -f1)

replace_line_index_1=$((replace_line_index_1_rel + line_index_1 - 1))
replace_line_index_2=$((replace_line_index_2_rel + line_index_1 - 1))

sed -i "${replace_line_index_1}s?"$'\t'"linux"$'\t'"\S*?"$'\t'"linux"$'\t'"/boot/vmlinuz-5\.11\.22?" /boot/grub/grub.cfg
sed -i "${replace_line_index_2}s?"$'\t'"initrd"$'\t'"\S*?"$'\t'"initrd"$'\t'"/boot/initrd\.img-5\.11\.22?" /boot/grub/grub.cfg




cd ..

rm ./linux-5.11.22.tar.xz
rm -rf ./linux-5.11.22

echo 
echo 
echo 
echo 
echo "Patched kernel install successfully!"
echo "Make sure to run stage 2 after reboot!"
#echo "And make sure to choose Advanced options for Ubuntu > Ubuntu, with Linux 5.11.22 in the grub menu! (or just don't touch anything for ~10 seconds)"
read -n 1 -p "(press enter to reboot)"

reboot
