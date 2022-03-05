#!/usr/bin/env python3

import subprocess, os, sys, re, time
import pexpect

user_id = int(subprocess.check_output(['id', '-u']))
user_dir = os.path.expanduser("~")
username = os.environ['USERNAME']
current_working_directory = os.getcwd()

default_run_command = "sudo -E python3 ./stage-3-pair-with-wiiu.py"

if user_id != 0:
    print("Please run with: %s" % default_run_command)
    sys.exit("Error: Script must be run as root.")

#if os.path.dirname(user_dir) != '/home':
#    print("Please run with: %s" % default_run_command)
#    sys.exit("Error: Parent directory of user directory is not /home")

if user_dir != '/home/'+username:
    print("Please run with: %s" % default_run_command)
    sys.exit("Error: User directory is not /home/"+username) 
    
if os.path.basename(current_working_directory) != 'pc2drc':
    if os.path.isdir(current_working_directory+'/drc-hostap'):
        print("Warning: Parent folder should be named pc2drc, continuing")
    else:
        print("This script expects to be in the pc2drc folder. Abort.")
        sys.exit("Error: script is not where it should be.")


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
    
    if os.path.exists("/sys/class/net/"+choice_wlan+"/tsf") != True:
        print("FATAL ERROR: TSF kernel patch not loaded.")
        input("(press enter to quit)")
        sys.exit(0)
    
    return choice_wlan
    
    
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
    

def pair_with_wii_u():
    while True:
        wifi_interface = choose_wifi_interface("Which wireless interface would you like to use to pair with the WiiU?")
        print("Choice: " + wifi_interface)
        
        while True:
            choice = input("Is this correct? (y/n): ")
            if choice != '':
                if choice[0].lower() == 'y':
                    confirmed = True
                    break
                elif choice[0].lower() == 'n':
                    confirmed = False
                    break
                
            input("Please input y or n. Press enter to try again.")
        
        if confirmed:
            break
        else:
            pass
                
                
    
    subprocess.check_call(['ip', 'link', 'set', 'dev', wifi_interface, 'up'])
    
    input("The next command will disable wifi. Press enter to continue or press Ctrl+C to cancel.")
    
    subprocess.check_call(['service', 'network-manager', 'stop'])
    
    subprocess.check_call(['ip', 'link', 'set', 'dev', wifi_interface, 'up'])
    
    subprocess.check_call(['cp', '-f', './drc-hostap/conf/get_psk.conf.orig', './drc-hostap/wpa_supplicant/get_psk.conf'])
    
    subprocess.check_call(['chown', '-R', username+':'+username, './drc-hostap/wpa_supplicant/get_psk.conf'])
    
    input("Turn on your WiiU, wait 15-20 seconds, press the red sync button on the faceplate, wait ~5 seconds, press the red sync button again, then press enter to continue.")
    
    def get_wps_pairing_code():
        print("There should be 4 symbols displayed from the WiiU. Each one of those corrosponds to a number.")
        while True:
            print("Spade = 0   Heart = 1   Diamond = 2   Club = 3")
            print("Example: ♠♦♣♦ = 0232")
            wps_pin_symbols = input("Decode the symbols on the WiiU and enter the 4-digit number here: ")
            
            if wps_pin_symbols.isdigit() and len(wps_pin_symbols) == 4:
                if int(wps_pin_symbols) >= 0:
                    wps_pin = str(wps_pin_symbols) + "5678"
                    break
                else:
                    input("Input must be a positive integer. Press enter to try again.")
            else:
                input("Input must be four digits. Press enter to try again.")
        return wps_pin
        
    wps_pin = get_wps_pairing_code()
            
    
    #print("DEBUG: wps_pin = %s" % wps_pin)
            
        
    
    #wpa_supplicant_output = subprocess.Popen(['./drc-hostap/wpa_supplicant/wpa_supplicant', '-Dnl80211', '-i'+wifi_interface, '-c./drc-hostap/wpa_supplicant/get_psk.conf'], stderr = subprocess.PIPE) 
    
    
    
    print("Starting wpa_supplicant...")
    wpa_supplicant_output = init_wpa_supplicant('./drc-hostap/wpa_supplicant/get_psk.conf', wifi_interface)
    
    #while True:
    #    print(wpa_supplicant_output.communicate(timeout=1)[0].decode("utf-8"))
    #    time.sleep(1)
    
    while True:
    
        ##wpa_cli_output = subprocess.Popen(['./drc-hostap/wpa_supplicant/wpa_cli', '-p/var/run/wpa_supplicant_drc'], stdin = subprocess.PIPE, stdout = subprocess.PIPE)
        print("Initializing scan...")
        #wpa_cli_output = pexpect.spawn('./drc-hostap/wpa_supplicant/wpa_cli -p/var/run/wpa_supplicant_drc')
        #wpa_cli_output.expect('Interactive mode\r\n\r\n> ')
        #while True:
        #    wpa_cli_output.sendline('scan')
        #    busy_status = wpa_cli_output.expect(['OK', 'FAIL-BUSY'])
        #    if busy_status == 0:
        #        break
        #    elif busy_status == 1:
        #        print("Device busy, trying again in 3 seconds...")
        #        time.sleep(3)
        #    else:
        #        print("Warning (Important): WHEEEEEEEEEEEEEEEEE")
        
        iw_scan = pexpect.spawn('/bin/bash -c "iw dev '+wifi_interface+' scan | grep -Eo \'WiiU[0-9a-f]{23}_STA1\'"')
        print("Scanning... (please be patient)")
        iw_scan.expect(pexpect.EOF, timeout=None)
        wii_u_line = iw_scan.before.decode('utf-8')
        
        #DEBUG (INFO): print(wii_u_line)
        
        if wii_u_line == "":
            input("WiiU not found or not in Gamepad pairing mode. Press the red sync button on the WiiU faceplate and press enter to try again.")
            #wpa_cli_output.kill(0)
            #time.sleep(2)
        else:
            if "WiiU" in wii_u_line:
                break
            else:
                print("There was an error while trying to scan for the WiiU. Try to run iw.")
                input("(press enter to quit)")
                sys.exit(0)
        
        
        #wpa_cli_output.expect('<3>CTRL-EVENT-SCAN-RESULTS')
        #time.sleep(0.5)
        #wpa_cli_output.sendline('scan_results')
        #wpa_cli_output.expect('> bssid / frequency / signal level / flags / ssid')
        #print("Scan complete! Fetching results...")
        #wpa_cli_output.expect('\r\n> ')
        #scan_results = wpa_cli_output.before.decode('utf-8')
        #scan_results_lines = scan_results.splitlines()
        #
        #wii_u_line = ""
        #
        #for line in scan_results_lines:
        #    if re.search("\w*	\[ESS\]	WiiU\w{23}_STA1\w*", line) != None:
        #        wii_u_line = line
        #        break
        #
        ##DEBUG (INFO): print(wii_u_line)
        #
        #if wii_u_line == "":
        #    input("WiiU not found or not in Gamepad pairing mode. Press the red sync button on the WiiU faceplate and press enter to try again.")
        #    wpa_cli_output.kill(0)
        #    time.sleep(2)
        #else:
        #    break
    
    
    #wii_u_bssid = wii_u_line[:17]
    
    wii_u_mac = wii_u_line[15:27] #WiiUaabbccddeefaabbccddeeff_STA1 -> aabbccddeeff = mac address
    
    wii_u_char_list = []
    
    for index in range(len(wii_u_mac)):
        wii_u_char_list.append(wii_u_mac[index])
        if not (index + 1) % 2:
            wii_u_char_list.append(':')
    wii_u_char_list.pop() #above function adds extra : to the end of the list.
    
    wii_u_bssid = ''.join(wii_u_char_list)
        
    
    print("WiiU Found!")
    
    print("WiiU BSSID: %s" % wii_u_bssid)
    
    wpa_cli_output = pexpect.spawn('./drc-hostap/wpa_supplicant/wpa_cli -p/var/run/wpa_supplicant_drc')
    
    try:
       wpa_cli_output.expect('Interactive mode\r\n\r\n> ', timeout=15)
    except:
       print("Fatal error: Could not start wpa_cli.")
       input("(press enter to quit)")
       sys.exit(0)
       
    
    while True:
        print("Pairing... (please be patient)")
        wpa_cli_output.sendline('wps_pin '+wii_u_bssid+' '+wps_pin)
        
        try:
            wpa_cli_output.expect("WPS-CRED-RECEIVED", timeout=10)
            try:
                wpa_cli_output.expect("WPS-SUCCESS", timeout=10)
                time.sleep(1)
                file = open("./drc-hostap/wpa_supplicant/get_psk.conf", 'r')
                get_psk_file = file.read()
                wii_u_psk = re.findall("	psk=[a-z0-9]{64}", get_psk_file)[0][5:]
                
                
                if wii_u_psk == None:
                    print("For some reason the WiiU's PSK was not obtained successfully. Restart the program and try again or email thefloppydriver@gmail.com for help :)")
                    input("(press enter to quit)")
                    sys.exit(0)
                    
                print("WiiU PSK obtained successfully.")
                print("Pairing Successful! Your WiiU console is not required for any later steps.")
                input("(press enter to continue)")
                break
            except:
                pass
        except:
            pass
        
        print("Pairing Unsuccessful. Try putting your WiiU back in Gamepad pairing mode.")
        while True:
            choice = input("Would you like to reenter the pairing code? (y/n): ")
            if choice != '':
                if choice[0].lower() == 'y':
                    wps_pin = get_wps_pairing_code()
                    break
                elif choice[0].lower() == 'n':
                    break
            input("Please input y or n. Press enter to try again.")
        
        
    
    
    
    
    #DEBUG (INFO): print("wii_u_psk= "+wii_u_psk)
    
    while True:
        choice = input("Would you like to use the same wireless interface to pair/stream to the WiiU Gamepad? (y/n): ")
        if choice != '':
            if choice[0].lower() == 'y':
                break
            elif choice[0].lower() == 'n':
                wifi_interface = choose_wifi_interface("Which wireless interface would you like to use to pair with the WiiU Gamepad?")
                break
        input("Please input y or n. Press enter to try again.")
            
    
    while True:
        file = open('./drc-hostap/conf/wiiu_ap_normal.conf.DONOTTOUCH', 'r')
        
        default_config = file.read()
        file.close()
        
        default_config = default_config.replace("ssid=ssid", "ssid=WiiU"+wii_u_bssid.lower().replace(':', ''))
        default_config = default_config.replace("wpa_psk=wpa_psk", "wpa_psk="+wii_u_psk)
        
        
        file = open('./drc-hostap/conf/generated_ap_normal.conf', 'w')
        file.write(default_config)
        file.close()
        
        if os.path.exists('./drc-hostap/conf/generated_ap_normal.conf'):
            print("Configuration saved successfully.")
            break
        else:
            print("Error: Unable to write configuration. Please restart the program.")
            input("(press enter to quit)")
            sys.exit(0)
    
    
    
    subprocess.check_call(['chown', '-R', username+':'+username, './drc-hostap/conf/generated_ap_normal.conf'])
    
    wpa_cli_output.sendline('terminate')
    time.sleep(3)
    wpa_cli_output.kill(0)
    
    return wifi_interface


#stage 2: pair with gamepad

def pair_with_gamepad(wifi_interface):
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
    
    #init_wpa_supplicant('./drc-hostap/conf/wpa_supplicant_normal.conf', wifi_interface)
    
    iw_reg_set_us_output = pexpect.spawn('iw reg set US')
    iw_reg_set_us_output.expect(pexpect.EOF)
    time.sleep(1)
    
    subprocess.check_call(['ip', 'a', 'a', '192.168.1.10/24', 'dev', 'WiiUAP'])
    subprocess.check_call(['ip', 'l', 'set', 'mtu', '1756', 'dev', 'WiiUAP'])
    
    time.sleep(1)
    
    
    subprocess.check_call(['ip', 'link', 'set', 'dev', wifi_interface, 'down'])
    
    time.sleep(1)
    
    hostapd_output = pexpect.spawn('./drc-hostap/hostapd/hostapd -dd ./drc-hostap/conf/generated_ap_normal.conf')
    
    
    time.sleep(1)
    
    print("Waiting for gamepad...")
    print("(please turn on the WiiU Gamepad)")
    hostapd_output.expect(' WPA: pairwise key handshake completed \(RSN\)', timeout=None)
    
    #pre_bssid = hostapd_output.before.decode('utf-8')
    #wii_u_gamepad_bssid = pre_bssid[-17:]
    
    wii_u_gamepad_bssid = hostapd_output.before.decode()[-17:]
    
    #hostapd_output.kill(0)
    
    print("Pairing Successful!")
    print("WiiU Gamepad BSSID: "+wii_u_gamepad_bssid)
    
    file = open('./drc-hostap/conf/generated_netboot_info.conf', 'w')
    file.write(wii_u_gamepad_bssid)
    file.close()
    
    if os.path.exists('./drc-hostap/conf/generated_netboot_info.conf'):
        print("Configuration saved successfully.")
    else:
        print("Error: Unable to write configuration. Please restart the program.")
        input("(press enter to quit)")
        sys.exit(0)
    
    
    subprocess.check_call(['chown', '-R', username+':'+username, './drc-hostap/conf/generated_netboot_info.conf'])
    
    
    netboot_output = pexpect.spawn('./drc-hostap/netboot/netboot 192.168.1.255 192.168.1.10 192.168.1.11 '+wii_u_gamepad_bssid)
    netboot_output.expect("request 3 from ")
    netboot_output.expect("matched")
    
    hostapd_output.kill(0)
    #wpa_supplicant_output.kill(0)
    print("WiiU Gamepad Paired and configured successfully! Onto stage four!")
    input("(press enter to quit)")
    
    
    sys.exit(0)
    
    
    
    
    
    
    
    
            
            
        
        
        
        
print("Type \"skip\" to skip Wii U pairing process. Press Enter to continue normally")
while True:
    choice = input("(type skip or press enter): ")
    if choice.lower() != '':
        if choice.lower() == 'skip':
            wifi_interface = choose_wifi_interface("Which wireless interface would you like to use to pair with the WiiU Gamepad?")
            subprocess.check_call(['service', 'network-manager', 'stop'])
            pair_with_gamepad(wifi_interface)
        else:
            print("Invalid choice.")
    else:
        wifi_interface = pair_with_wii_u()
        pair_with_gamepad(wifi_interface)
        

    

    
    
    
    
    
    
    
    
#print(wii_u_line)


#wpa_cli_output.communicate(b'scan_results\n')

#while True:
#    try:
#        print("\n\n\n\n\nwpa_supplicant output: "+wpa_supplicant_output.communicate(timeout=1)[0].decode("utf-8"))
#    except subprocess.TimeoutExpired as e:
#        print("\n\n\n\n\nwpa_supplicant output: Timed out")
#    
#    try:
#        print("\n\n\n\n\nwpa_cli output: "+wpa_cli_output.communicate(timeout=1)[0].decode("utf-8"))
#    except subprocess.TimeoutExpired as e:
#        print("\n\n\n\n\nwpa_cli output: Timed out")
#    time.sleep(1)

#while True:
#    try:
#        print("\n\n\n\n\nwpa_cli output: "+wpa_cli_output.communicate()[0].decode("utf-8"))
#        
#    except subprocess.TimeoutExpired as e:
#        print("\n\n\n\n\nwpa_cli output: Timed out")
#    
#        
#    #try:
#    #    print("\n\n\n\n\nwpa_supplicant output: "+wpa_supplicant_output.communicate(timeout=1)[0].decode("utf-8"))
#    #except subprocess.TimeoutExpired as e:
#    #    print("\n\n\n\n\nwpa_supplicant output: Timed out")
#        
#        
#    time.sleep(5)










#wlan = ip link show | grep -o -m1 "\w*: wl\w*" | grep -o "\w*wl\w*"







