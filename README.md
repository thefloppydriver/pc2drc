# pc2drc
 Stream PC->WiiU Gamepad and Give your gamepad a new use! (Linux only)
 
 
# What is this?
 An easy-to-setup way to stream your desktop directly from your PC to your WiiU Gamepad with full touchscreen/button support.
 
 
 # What do I need?
  - WiiU (Hopefully I'll be able to remove this requirement soon.)
  - WiiU Gamepad
  - **CLEAN** install of Ubuntu 20.04 LTS (original project used 18.04 LTS, it should work fine on there and I'll test it soon but lmk)
  802.11n 5GHz rt2800usb-compatable USB WiFi Adapter
    (RT5572-based adapters are the best, I'm using this and it works great -> https://www.amazon.com/Wireless-Portable-Frequency-Supports-Function/dp/B08LL94TK8)
  - ~15 GiB of storage space minimum. (why? Because we'll be recompiling the linux kernel, of course :)  )
    (Don't worry, the project only uses ~20MB after it's installed)
  - A little bit of patience and a willingness to send me bug reports if something goes wrong!
  
  
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
   
   
 # TODO:
   todo: add todo
   todo: add contributions
   todo: remove necessity for WiiU Gamepad
   
 
 # Notes:
  This project still has a lot of work to be done. Please fork this project, make changes, and make push requests. All contributions are welcome! 
  Email me: thefloppydriver@gmail.com
