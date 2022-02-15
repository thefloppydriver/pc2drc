Network configuration for Wii U GamePad <-> PC
==============================================

The Wii U GamePad communicates with other devices via the use of a slightly
obfuscated version of Wi-Fi 802.11n and WPA2 AES-CCMP. Luckily, the required
modifications for support of this obfuscated Wi-Fi protocol can be applied in
userland without any kernel modification.

Using libdrc to communicate with a Wii U GamePad however also requires the host
computer to get some information from its Wi-Fi interface which is not exported
by default on Linux. For this purpose, a kernel module needs to be built and
loaded on the machine to provide the required information.

Required hardware
-----------------

Connecting a Wii U GamePad to a computer requires compatible Wi-Fi hardware.
Any Wi-Fi NIC that can create access points on 5GHz 802.11 channels could
potentially work. In practice, the following drivers/NICs were tested:

* rt2800usb: works with Linux >= 3.12
* ath9k/carl9170: works with some caveats (communication might desync more
  often because of a TSF drifting issue)

Linux kernel patch
------------------

A very simple Linux kernel patch is required to export the Wi-Fi NIC *Time
Synchronization Function* (TSF) to userland. A patched version of the
``mac80211`` Linux module can be found on the ``memahaxx/drc-mac80211``
repository. It is based on Linux 3.12.3 but the patch should apply cleanly to
most recent Linux versions (e.g. Ubuntu > 13.10).

To check if the patched module is being used, try::

    test -f /sys/class/net/$WLANIFACE/tsf && echo ok

Pairing the Wii U GamePad with a computer
-----------------------------------------

There are two methods to pair a GamePad with a computer:

* Use a GamePad that has previously paired with a Wii U console. Connect the
  computer in client mode to the Wii U console to do the WPS negotiation and
  obtain the same details.

  1. Download and build ``memahaxx/drc-hostap``. Make sure ``CONFIG_WPS`` and
     ``CONFIG_TENDONIN`` are enabled for both hostapd and wpa_supplicant.
     Ensure that no other program (e.g. a system ``wpa_supplicant`` or
     ``NetworkManager`` is currently managing the target interface ``wlanX``).

  2. Make a new config ``get_psk.conf`` file consisting of these two lines::

         ctrl_interface=/var/run/wpa_supplicant_drc
         update_config=1

  3. In a terminal, execute::

         sudo ./wpa_supplicant -Dnl80211 -i wlanX -c get_psk.conf

     Replace ``wlanX`` with the correct network interface.

  4. In a separate terminal, execute::

         sudo ./wpa_cli -p /var/run/wpa_supplicant_drc

  5. Press the sync button twice on your Wii U console.

  6. In ``wpa_cli``, type::

         scan

  7. Once you see ``<3>CTRL-EVENT-SCAN-RESULTS`` type::

         scan_results

  8. On the returned list, you should see an entry with an SSID along the lines
     of ``WiiUAABBCCDDEEFF_STA1``. Take a note of the BSSID for this access
     point.

  9. In ``wpa_cli``, type::

         wps_pin BSSID PIN

     where ``BSSID`` is the BSSID you noted in the previous step, and ``PIN``
     is the PIN you calculated from the `Pairing information <http://libdrc.org/docs/re/wifi.html#pairing>`_.

  10. You should see some output, including ``<3>WPS-CRED-RECEIVED`` followed
      by ``<3>WPS-SUCCESS``.

  11. Ctrl+C ``wpa_supplicant``.

  12. To see your PSK, type::

          cat get_psk.conf

* Have a computer host WPS and sync a GamePad directly with it. TODO: write
  instructions for this method.

Setting up the Wi-Fi access point
---------------------------------

First of all, build the patched hostapd version available from
``memahaxx/drc-hostap``::

    git clone https://bitbucket.org/memahaxx/drc-hostap
    cd drc-hostap
    cp conf/hostapd.config hostapd/.config
    cd hostapd
    make -j4

Adapt the example configuration file from ``conf/wiiu_ap_normal.conf``:

* Change the network interface name
* Change the network SSID
* Change the PSK used for the network WPA2 security

Stop anything that might conflict with hostapd (for example, NetworkManager is
a known offender unless configured specifically to avoid this). Then start the
access point::

    sudo ./hostapd -dd ../conf/wiiu_ap_normal.conf

If everything worked well, hostapd should be waiting for devices to connect
instead of exiting immediately.

Make sure that no network interface is using the 192.168.1.0/24 network (it is
used for communication with the GamePad), then configure the interface::

    sudo ip a a 192.168.1.10/24 dev $IFACE
    sudo ip l set mtu 1800 dev $IFACE

Starting a DHCP server
----------------------

The GamePad uses DHCP to get an IP address from the Wii U or the PC it is
connected to. This IP address should always be 192.168.1.11. Any simple DHCP
server should work, but we recommend using netboot_, a very simple,
self-contained DHCP server.

.. _netboot: https://github.com/ITikhonov/netboot

Using netboot, the following command line should work (with the propre MAC
address of the GamePad)::

    ./netboot 192.168.1.255 192.168.1.10 192.168.1.11 aa-bb-cc-dd-ee-ff

From there, when powering on the GamePad, it should get an IP from netboot and
start sending packets to the computer. Using libdrc demos should work.
