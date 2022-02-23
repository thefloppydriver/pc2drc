#!/usr/bin/env python3

import subprocess, os, sys, re, time
import pexpect

user_id = int(subprocess.check_output(['id', '-u']))
user_dir = os.path.expanduser("~")
username = os.environ['USERNAME']
current_working_directory = os.getcwd()

default_run_command = "sudo -E python3 ./stage-4-start-pc2drc.py"
patched_kernel_version = "5.11.22"

if user_id != 0:
    print("Please run with: %s" % default_run_command)
    sys.exit("Error: Script must be run as root.")

if user_dir != '/home/'+username:
    print("Please run with: %s" % default_run_command)
    sys.exit("Error: User directory is not /home/"+username) 
    
if os.path.basename(current_working_directory) != 'pc2drc':
    if os.path.isdir(current_working_directory+'/libdrc-vnc'):
        print("Warning: Parent folder should be named pc2drc, continuing")
    else:
        print("This script expects to be in the pc2drc folder. Abort.")
        sys.exit("Error: script is not where it should be.")
        

    
    
def init_wpa_supplicant(config_file, wifi_interface):
        while True:
            wpa_supplicant_output = pexpect.spawn('./drc-hostap/wpa_supplicant/wpa_supplicant -Dnl80211 -i'+wifi_interface+' -c'+config_file)
            wpa_supplicant_output.expect('Successfully initialized wpa_supplicant', timeout=5)
        
            try:
                wpa_supplicant_output.expect(pexpect.EOF, timeout=3)
                #run the following if wpa_supplicant died
                try:
                    wpa_cli_output = pexpect.spawn('./drc-hostap/wpa_supplicant/wpa_cli -p/var/run/wpa_supplicant_drc')
                    wpa_cli_output.expect('Interactive mode\r\n\r\n> ')
                    wpa_cli_output.sendline('terminate')
                    wpa_cli_output.expect('OK')
                    wpa_cli_output.expect('<3>CTRL-EVENT-TERMINATING')
                    wpa_cli_output.kill(0)
                    time.sleep(0.5)
                except:
                    subprocess.check_call(['sudo', 'rm', '-r', '-f', '/var/run/wpa_supplicant_drc/'+wifi_interface])
            except:
                print("wpa_supplicant initialized successfully.")
                break
                
        return wpa_supplicant_output


def choose_wifi_interface(prompt_str):

    ip_link_output = subprocess.check_output(['ip', 'link', 'show']).decode("utf-8")

    ip_link_regex = re.findall("(\d{1}: wl[^<]+)", ip_link_output)

    wlan_array = []

    for wireless_interface in ip_link_regex:
        wlan_array.append(wireless_interface[3:-2])


    print("WiFi Interfaces:")
    for i in range(len(wlan_array)):
        print( "  %s: %s" % (str(i), wlan_array[i]) )
    
    print("\n"+prompt_str)

    while True:
        choice = input("Input (Number): ")
        if choice.isdigit():
            if int(choice) < len(wlan_array):
                choice_wlan = wlan_array[int(choice)]
                break
            else:
                input(choice + " is out of range. Press enter to try again.")
        else:
            input(choice + " is not a valid number. Press enter to try again.")
    
    
    return choice_wlan
    
    
#network_ssid = subprocess.check_output(['iwgetid', '-r']).decode('utf-8')[:-1]
#secrets_output = subprocess.check_output(['nmcli', '--show-secrets', 'connection', 'show', 'id', network_ssid]).decode('utf-8')
#network_psk = re.findall("802-11-wireless-security\.psk.*", secrets_output)[0][40:]


wifi_interface = choose_wifi_interface("Which wireless interface would you like to use to stream to the WiiU Gamepad?")

if os.path.exists("/sys/class/net/"+wifi_interface+"/tsf") != True:
    print("TSF kernel patch not loaded.")
    if subprocess.check_output(['uname', '-r']).decode('utf-8') != patched_kernel_version:
        print("You are running an unpatched kernel.")
        print("Please reboot to grub and choose Advanced options for Ubuntu > Ubuntu, with Linux 5.11.22   (It should be the selected by default)")
        input("(press enter to quit")
        sys.exit(0)
    #else:
    print("You are running the patched kernel version, but the patch isn't working.")
    print("Please reboot and try again or email thefloppydriver@gmail.com for help.")
    input("(press enter to quit)")
    sys.exit(0)
    
#else:




#iw_reg_set_us_output = pexpect.spawn('iw reg set US')
#iw_reg_set_us_output.expect(pexpect.EOF)
#time.sleep(1)


subprocess.check_call(['service', 'network-manager', 'stop'])

#TESTING
#TODO: test rtnetlink directly instead of testing with ip link set dev wifi_interface up
#TODO: check if wpa_supplicant and hostapd are alive before doing this test
try:
    #TODO: test rtnetlink directly instead of using this
    subprocess.check_call(['ip', 'link', 'set', 'dev', wifi_interface, 'up'])
except subprocess.CalledProcessError:
    print("Wireless interface is being used, killing wpa_supplicant...")
    try:
        subprocess.check_call(['killall', 'wpa_supplicant'])
    except subprocess.CalledProcessError:
        print("wpa_supplicant was not alive...")

try:
    #TODO: test rtnetlink directly instead of using this
    subprocess.check_call(['ip', 'link', 'set', 'dev', wifi_interface, 'up'])
except subprocess.CalledProcessError:
    print("Wireless interface is being used, killing hostapd...")
    try:
        subprocess.check_call(['killall', 'hostapd'])
    except subprocess.CalledProcessError:
        print("hostapd was not alive...")
        
try:
    #TODO: test rtnetlink directly instead of using this
    subprocess.check_call(['ip', 'link', 'set', 'dev', wifi_interface, 'up'])
except subprocess.CalledProcessError:
    print("Wireless interface is STILL being used.")
    print("Could not get a lock on wifi interface ("+wifi_interface+")")
    print("Please stop anything that could be using the interface and try again.")
    input("(press enter to quit)")
#END TESTING



#def pair_with_gamepad(wifi_interface):
while True:
    ip_link_output = subprocess.check_output(['ip', 'link', 'show']).decode("utf-8")
    ip_link_regex = re.findall("(\d{1}: [^<]+)", ip_link_output)
    wlan_array = []
    for wireless_interface in ip_link_regex:
        wlan_array.append(wireless_interface[3:-2])
    
    if 'WiiUAP' in wlan_array:
        subprocess.check_call(['iw', 'dev', 'WiiUAP', 'del'])
        time.sleep(1)
    
    subprocess.check_call(['iw', 'dev', wifi_interface, 'interface', 'add', 'wlWiiUAP', 'type', 'managed'])
    
    time.sleep(1)
    
    ip_link_output = subprocess.check_output(['ip', 'link', 'show']).decode("utf-8")
    ip_link_regex = re.findall("(\d{1}: [^<]+)", ip_link_output)
    wlan_array_2 = []
    for wireless_interface in ip_link_regex:
        wlan_array_2.append(wireless_interface[3:-2])
     
    
    wii_u_ap_devs = list(set(wlan_array_2) - set(wlan_array))
    
    if len(wii_u_ap_devs) > 1:
        input("well that didn't work. unplug your WiiU AP device, plug it back in, then press enter to try again :)")
    else:
        wii_u_ap_dev = wii_u_ap_devs[0]
        subprocess.check_call(['ip', 'link', 'set', wii_u_ap_dev, 'name', 'WiiUAP'])
        break





print("Starting VNC server...")
#subprocess.check_call(['tigervncserver', '--kill', ':1']) 

time.sleep(0.5)
#os.system("sudo runuser -u thefloppydriver -- /bin/bash -c \"echo -e '12345678\\n12345678\\nn' | vncpasswd\"") #unnecessary
#subprocess.check_call(['chmod', '0664', user_dir+'/.vnc/passwd'])
#os.system("sudo runuser -u thefloppydriver -- tigervncserver :1 -passwd "+user_dir+"/.vnc/passwd -depth 24 -geometry 640x480 -localhost yes")
try:
    os.system("sudo runuser -u thefloppydriver -- tigervncserver :1 -useold -FrameRate 59.94 -SecurityTypes None -depth 24 -geometry 640x480 -localhost yes -xstartup "+current_working_directory+"/libdrc-vnc/vncconfig-files/Xvnc-session-gnome")
except:
    print("Failed to start tigervncserver, killing and trying again.")
    subprocess.check_call(['tigervncserver', '--kill', ':1'])
    try:
        os.system("sudo runuser -u thefloppydriver -- tigervncserver :1 -useold -FrameRate 59.94 -SecurityTypes None -depth 24 -geometry 640x480 -localhost yes -xstartup "+current_working_directory+"/libdrc-vnc/vncconfig-files/Xvnc-session-gnome")
    except:
        print("Could not start tigervncserver.")
        input("(press enter to quit)")
        sys.exit(0)

#try:
#    vncserver.expect(pexpect.EOF, timeout=30)
#except:
#    print("Unable to start VNC server.")
#    input("(press enter to quit)")
#    sys.exit(0)


print("VNC Server started successfully.")



time.sleep(1) #???



#vncviewer = pexpect.spawn("xtigervncviewer -SecurityTypes VncAuth -passwd "+user_dir+"/.vnc/passwd :1 -DesktopSize=854x480")

# This is an awful, awful thing to do, but it's the only reasonable way to get the desktop to resize to this resolution. why me. -thefloppydriver
vncviewer = pexpect.spawn("xtigervncviewer -SecurityTypes None :1 -DesktopSize=854x480")

try:
    vncviewer.expect(" Connected to host", timeout=10)
except:
    print(vncserver.before.decode('utf-8'))
    print("\n\n\n\n\n"+vncviewer.before.decode('utf-8'))
    print("Could not connect to vncserver. Please restart the program and try again.")
    subprocess.check_call(['tigervncserver', '--kill', ':1'])
    input("(press enter to quit)")
    sys.exit(0)
    
time.sleep(0.25)
vncviewer.kill(0) #this likes to misbehave, hence the following line.
subprocess.check_call(['killall', 'xtigervncviewer'])
#subprocess.check_call(['tigervncserver', '--kill', ':1'])
time.sleep(0.5)





#print("Waiting for gamepad...")
#print("(please turn on the WiiU Gamepad)")
#hostapd_output.expect(' WPA: pairwise key handshake completed \(RSN\)', timeout=None)
#wii_u_gamepad_bssid = hostapd_output.before.decode()[-17:]
#print("Pairing Successful!")
#print("WiiU Gamepad BSSID: "+wii_u_gamepad_bssid)



if os.path.exists('./drc-hostap/conf/generated_netboot_info.conf'):
    file = open('./drc-hostap/conf/generated_netboot_info.conf', 'r')
    wii_u_gamepad_bssid = file.read(17) #because the bssid is only 17 chars
    file.close()
else:
    print("Gamepad configuration not found. Please run stage 3.")
    input("(press enter to quit)")
    sys.exit(0)


try:
    subprocess.check_call(['killall', 'hostapd']) #TODO: make this less extreme
except subprocess.CalledProcessError:
    pass

iw_reg_set_us_output = pexpect.spawn('iw reg set US')
iw_reg_set_us_output.expect(pexpect.EOF)
time.sleep(1)

subprocess.check_call(['ip', 'link', 'set', 'dev', wifi_interface, 'up'])

time.sleep(1)

print("Initializing wpa_supplicant...")
wpa_suppy = init_wpa_supplicant('./drc-hostap/conf/wpa_supplicant_normal.conf', wifi_interface)

time.sleep(1)

subprocess.check_call(['ip', 'link', 'set', 'dev', wifi_interface, 'down'])

time.sleep(1)

#hostapd_output = pexpect.spawn('./drc-hostap/hostapd/hostapd -dd ./drc-hostap/conf/generated_ap_normal.conf')
try:
    subprocess.check_call(['./drc-hostap/hostapd/hostapd', '-B', './drc-hostap/conf/generated_ap_normal.conf'])
except subprocess.CalledProcessError:
    print("Fatal Error while starting hostapd.")
    input("(press enter to quit)")
    sys.exit(0)


time.sleep(1)

subprocess.check_call(['ip', 'a', 'a', '192.168.1.10/24', 'dev', 'WiiUAP'])
subprocess.check_call(['ip', 'l', 'set', 'mtu', '1756', 'dev', 'WiiUAP'])

time.sleep(1)

file = open("/sys/class/net/"+wifi_interface+"/tsf", 'rb')
file_output = file.read(8)
if file_output == b'\x00\x00\x00\x00\x00\x00\x00\x00' or file_output == b'\xff\xff\xff\xff\xff\xff\xff\xff':
    print("Selected wireless interface ("+wifi_interface+" is not exporting TSF value.")
    print("Either the wireless interface doesn't support mac80211 get_tsf function or the kernel patch isn't working.")
    input("(press enter to quit)")
    sys.exit(0)


print("Waiting for gamepad...")
print("(please turn on the WiiU Gamepad)")

keep_quitting=False

while True:
    netboot_output = pexpect.spawn('./drc-hostap/netboot/netboot 192.168.1.255 192.168.1.10 192.168.1.11 '+wii_u_gamepad_bssid)
    try:
        netboot_output.expect("request 1 from ", timeout=5)
        print("Gamepad found! Connecting...")
        try:
            netboot_output.expect("request 3 from ", timeout=15)
            print("Connected to gamepad!")
            keep_quitting=True
            break
        except pexpect.exceptions.TIMEOUT:
            print("Couldn't connect to gamepad, retrying...")
            
        if keep_quitting:
            break
    except pexpect.exceptions.TIMEOUT:
        print("Didn't find gamepad, retrying...")
    


        

        

#netboot_output.expect("matched")
#print("matched...")
time.sleep(0.5)

print("WiiU Gamepad booted successfully!")

print("Starting drcvncclient!")




#pair_with_gamepad(wifi_interface)

subprocess.check_call(['chmod', '777', '/dev/uinput']) #make sure that drcvncclient can write to this


#subprocess.check_call(['cp', './stage-4-start-pc2drc.py', './start-pc2drc.py'])

#print("start drcvncclient manually now.")
#time.sleep(1000)

#todo: start drcvncclient, check for errors, create startup script

#print("do the thing")
#time.sleep(1000)

#os.system("/bin/bash -c \"./libdrc-vnc/drcvncclient/src/drcvncclient -joystick :1 >/dev/null <<EOFwhyisthisathing\nEOF\n\"")

#print("Password is: 12345678")
os.system("./libdrc-vnc/drcvncclient/src/drcvncclient :1") #-joystick :1")

#whyisthisathing


#todo: create startup script for pc2drc :)
