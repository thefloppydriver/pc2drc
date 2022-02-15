#!/bin/bash

if [ `id -u` -ne 0 ]; then
    echo "Please run with: sudo -E ./stage-1-kernel-patch.sh"
    exit
fi

if [ $(cd $HOME/.. && pwd) != "/home" ]; then
    echo "Please run with: sudo -E ./stage-1-kernel-patch.sh"
    exit
fi



apt-get install git fakeroot build-essential ncurses-dev xz-utils libssl-dev bc flex libelf-dev bison -y

#mkdir kernel-patch-files #todo, add gitignore linux-5.11.22.tar.xz and linux-5.11.22
cd kernel-patch-files 

wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.11.22.tar.xz
tar xvf linux-5.11.22.tar.xz


cp -v /boot/config-$(uname -r) ./linux-5.11.22/.config

cd ./linux-5.11.22/net/mac80211
patch -p1 < ../../../mac80211.patch #add sudo if this doesn't work
cd -

sed -i 's/CONFIG_SYSTEM_TRUSTED_KEYS/#CONFIG_SYSTEM_TRUSTED_KEYS/g' ./linux-5.11.22/.config

cd linux-5.11.22
make -j`nproc` #this *will* take a while. 

make modules_install
make install

cp /etc/default/grub /etc/default/grub.backupfilebaybee

sed -i -e 's/GRUB_DEFAULT\=0/GRUB_DEFAULT\="Advanced\ options\ for\ Ubuntu\>Ubuntu,\ with Linux\ 5.11.22"/' /etc/default/grub

update-grub

cd ..

rm ./linux-5.11.22.tar.xz
rm -rf ./linux-5.11.22

echo 
echo 
echo 
echo 
echo "Patched kernel install successfully!"
echo "Make sure to run stage 2 after reboot!"
echo "And make sure to choose Advanced options for Ubuntu > Ubuntu, with Linux 5.11.22 in the grub menu! (or just don't touch anything for ~10 seconds)"
read -n 1 -p "(press enter to reboot)"

reboot
