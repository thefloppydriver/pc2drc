# pc2drc
 Stream PC->WiiU Gamepad and Give your gamepad a new use! (Linux only)
 
 
# HUGE DISCLAIMER:
 THIS IS NOT FINISHED YET!!
 
 DO NOT CLONE/RUN THIS PROJECT UNTIL I REMOVE THIS DISCLAIMER (will be removed Feb 19th, 2022)
 
# What is this?
 An easy-to-setup way to stream your desktop directly from your PC to your WiiU Gamepad with almost 0 latency and full touchscreen/joystick support!
 
 
 # What do I need?
  - WiiU (Currently working on removing this requirement. Will update when finished.)
  - WiiU Gamepad
  - **CLEAN** install of Ubuntu 20.04 LTS (original project used 18.04 LTS, it should work fine on there and I'll test it soon but lmk)
  - 802.11n 5GHz rt2800usb-compatable USB WiFi Adapter
     (RT5572-based adapters are known to work, I'm using this and it works great -> https://www.amazon.com/Wireless-Portable-Frequency-Supports-Function/dp/B08LL94TK8)
     NOTE: I am in the process of getting my hands on a newer Mediatek adapter that supports get_tsf() in it's kernel drivers. I will update this after testing it.
  - ~15 GiB of storage space minimum. (why? Because we'll be recompiling the linux kernel, of course :)  <- *might* change soon.)
     (Don't worry, the entire project only uses ~20MB after it's installed)
  - A little* bit of patience and a willingness to send me bug reports if something goes wrong!
  
  
 # Dependencies
  Installing extra software is so 2010. I'll add a list later but there's nothing extra to worry about installing, the scripts install everything they need automatically without much fuss.
  
  
 # (How to install) The WiFi adapter just came in, now what?
  Open up your home directory (or where ever you want to install the project)
  Run the following commands in order and follow their instructions:
  
   `sudo -E ./stage-0-start-script.sh`
   
   `sudo -E ./stage-1-kernel-patch.sh`
   
   `sudo -E ./stage-2-build-modules.sh`
   
   `sudo -E python3 ./stage-3-pair-with-wiiu.py`
   
   `sudo -E python3 ./stage-4-start-pc2drc.py`
   
  Once finished, simply run `sudo -E python3 ./start-pc2drc.py` to stream to your gamepad!
   
   
 # Extra Instructions and tutorials:
  Check out my YT https://www.youtube.com/channel/UCeQyKqdmkixb4vK9WuUJGFg
  
  Setup Video: (coming soon)
   
   
 # TODO: (in order of importance)
   ~~todo: add todo~~
   
   todo (unfinished): finish part-4-pc2drc.py
   
   todo (unfinished): create start-pc2drc.py
   
   todo (feature): remove necessity for WiiU during pairing (seemingly easy)
   
   todo (QoL, stability): Upgrade wpa_supplicant and hostapd and remodify them for WiiU support (Hard? Probably?)
   
   todo (QoL, stability): find better way to resize the tigervnc-server that doesn't involve connecting with xtigervncclient (stuff goes here) -DesktopSize 864x480
   
   todo (misc.): add contributions and links to software used in this project
   
   todo (QoL, stability): remake wpa_cli.c to only output WiiU related information
   
   todo (feature): Port to openssl 1.1
   
   todo (feature): Port to openssl 3.0
   
   todo (feature): do mac80211 patch through DKMS using FlorianFranzen's drc-mac80211 repository (Previously known as FloFra. God he was hard to find. To anyone reading this please do not ever rename your account.)
 
   todo (feature): Maybe spam reset_tsf to allow something around 00000000 to always be set as tsf value when talking to gamepad
 
 # Known Bugs: 
  
  (fix unimplemented) Pairing with the WiiU takes a *longgg* time.
  
  (fix unknown) Connection between PC and Gamepad randomly (maybe error in network stack, maybe decoding error, maybe encoding error)
  
  (fix unknown) Visible artifacting on gamepad (x264 encoding bug, this is NOT a network related issue)
   
 
 # Notes:
  This project still has a lot of work to be done. Please fork this project, make changes, and make pull requests. All contributions are welcome! 
  
  Email me: thefloppydriver@gmail.com
