#!/bin/bash

if [ `id -u` -ne 0 ]; then
    echo "Please run with: sudo -E ./stage-2-build-modules.sh"
    exit
fi

if [ $(cd $HOME/.. && pwd) != "/home" ]; then
    echo "Please run with: sudo -E ./stage-2-build-modules.sh"
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

if [ ${PWD##*/} != "pc2drc" ]; then
  echo "Parent folder is not named pc2drc. If this script isn't its intended directory you will have an unusable system."
  read -n 1 -p "(press enter to quit)"
  exit
fi

echo -e "GET http://google.com HTTP/1.0\n\n" | nc google.com 80 > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "An internet connection is required for this script to run. Try sudo service network-manager start."
    read -n 1 -p "(press enter to quit)"
    exit
fi

script_dir=$(pwd)


apt --fix-broken install -y

apt-get install libnl-3-dev libnl-genl-3-dev -y

apt-get install ffmpeg libswscale-dev libavutil-dev -y

apt-get install yasm mesa-utils freeglut3 freeglut3-dev libglew-dev libgl1-mesa-dev libsdl1.2-dev libsdl2-dev tigervnc-standalone-server tigervnc-viewer cmake python3 -y
apt-get remove libx264-dev -y

mkdir -m 775 ./external_pkg_build_dir
mkdir -m 775 ./external_pkg_install_dir

chown -R $USERNAME:$USERNAME ./external_pkg_build_dir
chown -R $USERNAME:$USERNAME ./external_pkg_install_dir

rm -rf ./external_pkg_install_dir/* #careful with this

cd ./external_pkg_build_dir
wget https://ftp.openssl.org/source/old/1.0.1/openssl-1.0.1u.tar.gz --no-check-certificate
git clone https://github.com/LibVNC/libvncserver.git



tar -xf ./openssl-1.0.1u.tar.gz
rm ./openssl-1.0.1u.tar.gz*
cd ./openssl-1.0.1u

#~/Documents/bring-back-libdrc/bring-back-libdrc/drc-hostap/hostapd/Makefile Line 475:  LIBS += -lssl -lcrypto -lz -ldl
#~/Documents/bring-back-libdrc/bring-back-libdrc/drc-hostap/wpa_supplicant/Makefile Line 927:  LIBS += -lcrypto -lz
#~/Documents/bring-back-libdrc/bring-back-libdrc/drc-hostap/wpa_supplicant/Makefile Line 928:  LIBS += -lcrypto -lz -ldl

make clean -j`nproc`

./config --prefix=$script_dir/external_pkg_install_dir/ --openssldir=openssl threads zlib no-shared
make -j`nproc`
make install

cd ..


#echo "TODO: COMPILE AND INSTALL LIBVNCSERVER/LIBVNCCLIENT"

mkdir ./libvncserver/build

cd ./libvncserver/build

make clean
cmake ..
cmake --build . --parallel `nproc`
make install

cd ../..

rm -rf ./libvncserver #careful
rm -rf ./openssl-1.0.1u #careful


#wget https://launchpad.net/~ubuntu-security/+archive/ubuntu/ppa/+build/7531893/+files/openssl_1.0.1-4ubuntu5.31_amd64.deb --no-check-certificate
#wget https://launchpad.net/~ubuntu-security/+archive/ubuntu/ppa/+build/7531893/+files/libssl1.0.0_1.0.1-4ubuntu5.31_amd64.deb --no-check-certificate
#wget https://launchpad.net/~ubuntu-security/+archive/ubuntu/ppa/+build/7531893/+files/libssl-dev_1.0.1-4ubuntu5.31_amd64.deb --no-check-certificate

#wget -c http://archive.ubuntu.com/ubuntu/pool/universe/libn/libnl/libnl1_1.1-8ubuntu1_amd64.deb
#wget -c http://archive.ubuntu.com/ubuntu/pool/universe/libn/libnl/libnl-dev_1.1-8ubuntu1_amd64.deb

#dpkg --ignore-depends=multiarch-support -i libssl1.0.0_1.0.1-4ubuntu5.31_amd64.deb
#dpkg -i openssl_1.0.1-4ubuntu5.31_amd64.deb
#dpkg -i libssl-dev_1.0.1-4ubuntu5.31_amd64.deb



cd ..

rm -d ./external_pkg_build_dir


#####git clone https://bitbucket.org/memahaxx/drc-hostap.git
#####cd drc-hostap
#####git clone https://github.com/ITikhonov/netboot.git

cd ./drc-hostap/netboot
rm ./netboot
gcc -o netboot netboot.c

chown $USERNAME:$USERNAME ./netboot
cd ../..

cd ./drc-hostap/hostapd

cp -f ./defconfig ./.config

sed -i "s?/_123sedreplaceme?${script_dir}?g" ./.config

make clean
make -j`nproc`

cd ../..

chown -R $USERNAME:$USERNAME ./drc-hostap/hostapd/*





#apt --fix-broken install -y

#apt remove libnl-3-dev -y

#dpkg --ignore-depends=multiarch-support -i ./libnl1_1.1-8ubuntu1_amd64.deb
#dpkg --ignore-depends=multiarch-support -i ./libnl-dev_1.1-8ubuntu1_amd64.deb



#dpkg --ignore-depends=multiarch-support -i libssl1.0.0_1.0.1-4ubuntu5.31_amd64.deb
#dpkg -i libssl-dev_1.0.1-4ubuntu5.31_amd64.deb
#dpkg -i openssl_1.0.1-4ubuntu5.31_amd64.deb



#rm ./openssl_1.0.1-4ubuntu5.31_amd64.deb
#rm ./libssl1.0.0_1.0.1-4ubuntu5.31_amd64.deb
#rm ./libssl-dev_1.0.1-4ubuntu5.31_amd64.deb
#rm ./external_pkg_build_dir/libnl1_1.1-8ubuntu1_amd64.deb*
#rm ./external_pkg_build_dir/libnl-dev_1.1-8ubuntu1_amd64.deb*

#apt --fix-broken install -y

cd ./drc-hostap/wpa_supplicant

cp -f ./defconfig ./.config

sed -i "s?/_123sedreplaceme?${script_dir}?g" ./.config

make clean
make -j`nproc`

cd ../..

chown -R $USERNAME:$USERNAME ./drc-hostap/wpa_supplicant/*



#apt --fix-broken install


#echo "TODO: COMPILE AND INSTALL MODIFIED FFMPEG AND DRC-X264"

cd ./drc-x264
make clean
./configure --prefix=/usr/local --enable-static --enable-pic # --enable-shared
make -j`nproc`
make install -j`nproc`

cd ..


#echo "TODO: COMPILE AND INSTALL MODIFIED LIBDRC"

cd ./libdrc-vnc/libdrc-thefloppydriver

make clean -j`nproc`
PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH ./configure --disable-demos --disable-debug --prefix=/usr/local

make -j`nproc`
make install -j`nproc`

cd ../..

chown -R $USERNAME:$USERNAME ./libdrc-vnc/libdrc-thefloppydriver/*
chown -R $USERNAME:$USERNAME ./libdrc-vnc/libdrc-thefloppydriver/src/*







#echo "TODO: COMPILE AND INSTALL A VNC VIEWER"

#tigervnc-standalone-server was (hopefully) already installed earlier in the script.


#vncpasswd
mkdir ~/.vnc

#tigervnc startup script that inits for a gamepad connection

#printf '#!/bin/sh\nxrdb $HOME/.Xresources\nxsetroot -solid grey\nexport XKL_XMODMAP_DISABLE=1\n/etc/X11/Xsession\n[ -x /etc/vnc/xstartup ] && exec /etc/vnc/xstartup\n[ -r $HOME/.Xrespirces ] && xrdb $HOME/.Xresources\nvncconfig -iconic &\ndbus-launch --exit-with-session gnome-session &\nsleep 1 && xrandr --fb 854x480 &' > ~/.vnc/xstartup
printf '#!/bin/sh\ntest x"$SHELL" = x"" && SHELL=/bin/bash\ntest x"$1"     = x"" && set -- default\nvncconfig -iconic &\n"$SHELL" -l <<EOF\nexport XDG_SESSION_TYPE=x11\ndbus-launch --exit-with-session gnome-session\nexec /etc/X11/Xsession "$@"\nEOF\nvncserver -kill $DISPLAY' > /etc/vnc/xstartup
printf '#!/bin/sh\n[ -x /etc/vnc/xstartup ] && exec /etc/vnc/xstartup\n[ -r $HOME/.Xresources ] && xrdb $HOME/.Xresources\nsleep 1 && xrandr --fb 854x480 &' > ~/.vnc/xstartup

/bin/bash -c "echo -e '\n\nn' | vncpasswd"; echo

chmod 0755 /etc/vnc/xstartup
chmod 0755 ~/.vnc/xstartup
chmod 0664 ~/.vnc/passwd

#echo | vncpasswd -f > ~/.vnc/passwd

chown -R $USERNAME:$USERNAME ~/.vnc

#tigervncserver :1 -passwd ~/.vnc/passwd -geometry 1280x720 -depth 24 -localhost
#xtigervncviewer :1 -passwd ~/.vnc/passwd -DesktopSize=864x480




#echo "TODO: COMPILE AND INSTALL drcvncclient"

cd ./libdrc-vnc/drcvncclient

make clean

x264_LIBS='-L/usr/local/lib -lx264 -L/usr/local/lib -lswscale' ./configure
make -j`nproc`

cd ../..













