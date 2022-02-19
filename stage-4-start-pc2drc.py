#!/usr/bin/env python3

import subprocess, os, sys, re, time
import pexpect

user_id = int(subprocess.check_output(['id', '-u']))
user_dir = os.path.expanduser("~")
username = os.environ['USERNAME']

default_run_command = "sudo -E python3 ./stage-4-start-pc2drc.py"
patched_kernel_version = "5.11.22"

if user_id != 0:
    print("Please run with: %s" % default_run_command)
    sys.exit("Error: Script must be run as root.")

if os.path.dirname(user_dir) != '/home':
    print("Please run with: %s" % default_run_command)
    sys.exit("Error: Parent directory of user directory is not /home")
    
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



iw_reg_set_us_output = pexpect.spawn('iw reg set US')
iw_reg_set_us_output.expect(pexpect.EOF)
time.sleep(1)

wpa_suppy = init_wpa_supplicant('./drc-hostap/conf/wpa_supplicant_normal.conf', wifi_interface)

subprocess.check_call(['ip', 'link', 'set', 'dev', wifi_interface, 'down'])

time.sleep(1)

hostapd_output = pexpect.spawn('./drc-hostap/hostapd/hostapd -dd ./drc-hostap/conf/generated_ap_normal.conf')

time.sleep(1)

subprocess.check_call(['ip', 'a', 'a', '192.168.1.10/24', 'dev', 'WiiUAP'])
subprocess.check_call(['ip', 'l', 'set', 'mtu', '1756', 'dev', 'WiiUAP'])

time.sleep(1)

print("Waiting for gamepad...")
print("(please turn on the WiiU Gamepad)")
hostapd_output.expect(' WPA: pairwise key handshake completed \(RSN\)', timeout=None)
    
#pre_bssid = hostapd_output.before.decode('utf-8')
#wii_u_gamepad_bssid = pre_bssid[-17:]

wii_u_gamepad_bssid = hostapd_output.before.decode()[-17:]

##hostapd_output.kill(0)

print("Pairing Successful!")
print("WiiU Gamepad BSSID: "+wii_u_gamepad_bssid)

#file = open('./drc-hostap/conf/generated_netboot_info.conf', 'w')
#file.write(wii_u_gamepad_bssid)
#file.close()
#
#if os.path.exists('./drc-hostap/conf/generated_netboot_info.conf'):
#    print("Configuration saved successfully.")
#else:
#    print("Error: Unable to write configuration. Please restart the program.")
#    input("(press enter to quit)")
#    sys.exit(0)
#
#
#subprocess.check_call(['chown', '-R', username+':'+username, './drc-hostap/conf/generated_netboot_info.conf'])


netboot_output = pexpect.spawn('./drc-hostap/netboot/netboot 192.168.1.255 192.168.1.10 192.168.1.11 '+wii_u_gamepad_bssid)
netboot_output.expect("request 3 from ")
netboot_output.expect("matched")

#hostapd_output.kill(0)
##wpa_supplicant_output.kill(0)
print("WiiU Gamepad Paired and configured successfully!")
#input("(press enter to quit)")



#pair_with_gamepad(wifi_interface)

subprocess.check_call(['chmod', '777', '/dev/uinput']) #make sure that drcvncclient can write to this


print("Starting VNC server...")
subprocess.check_call(['tigervncserver', '--kill', ':1']) 

time.sleep(0.5)
os.system("sudo runuser -u thefloppydriver -- /bin/bash -c \"echo -e '1234\\n1234\\nn' | vncpasswd\"")
subprocess.check_call(['chmod', '0664', user_dir+'/.vnc/passwd'])
os.system("sudo runuser -u thefloppydriver -- tigervncserver :1 -passwd "+user_dir+"/.vnc/passwd -depth 24 -geometry 640x480 -localhost yes")

#try:
#    vncserver.expect(pexpect.EOF, timeout=30)
#except:
#    print("Unable to start VNC server.")
#    input("(press enter to quit)")
#    sys.exit(0)


print("VNC Server started successfully.")



time.sleep(1) #???



vncviewer = pexpect.spawn("xtigervncviewer -SecurityTypes VncAuth -passwd "+user_dir+"/.vnc/passwd :1 -DesktopSize=854x480")

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
vncviewer.kill(0)
subprocess.check_call(['killall', 'xtigervncviewer'])
#subprocess.check_call(['tigervncserver', '--kill', ':1'])
time.sleep(0.5)

#print("start drcvncclient manually now.")
#time.sleep(1000)

#todo: start drcvncclient, check for errors, create startup script

#print("do the thing")
#time.sleep(1000)

#os.system("/bin/bash -c \"./libdrc-vnc/drcvncclient/src/drcvncclient -joystick :1 >/dev/null <<EOFwhyisthisathing\nEOF\n\"")

print("Password is: 1234")
os.system("./libdrc-vnc/drcvncclient/src/drcvncclient -joystick :1")

#whyisthisathing


#todo: create startup script for pc2drc :)
