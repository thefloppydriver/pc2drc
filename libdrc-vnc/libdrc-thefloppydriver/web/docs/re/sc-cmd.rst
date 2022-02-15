Command protocol
================

+--------------+----------+---------------+
| Console Port | Pad Port | Direction     |
+==============+==========+===============+
| 50023        | 50123    | Console ↔ Pad |
+--------------+----------+---------------+

The command protocol, also known as ``cmd``, is used as a lightweight RPC
between the Wii U and a GamePad. Using this protocol, the Wii U can send
commands to a GamePad and get a reply from it. This protocol has various uses,
from firmware upgrade to NFC communication.

Protocol state machine
----------------------

Each ``cmd`` request has 4 states which are encoded in the sent messages:

* DRH sends a query message (packet type 0) to DRC
* DRC acks the query (packet type 1)
* DRC sends the reply (packet type 2)
* DRH acks the reply (packet type 3)

Until a ACK is received, DRH/DRC will retry sending.

Main protocol header
--------------------

.. code-block:: c

    struct CmdHeader {
        le16 packet_type;
        le16 query_type;
        le16 payload_size;
        le16 seq_id;
    };

Packet type
    See the state machine documentation above.

Query type
    The query type describes what the ``cmd`` payload will look like. 3 query
    types exist:

    * Generic command (query type 0)
    * UVC/UAC command (query type 1)
    * Time command (query type 2)

Payload size
    Should be ``packet_size - sizeof (CmdHeader)``.

Sequence ID
    Incremented for each new request sent from DRH. Stays the same between
    packets of the same request.

This header is followed by a payload of varying type, depending on the query
type field.

Generic command header
----------------------

Generic commands are the most used ``cmd`` requests. They allow access to most
of the features of the GamePad not already exported through the input protocol.
It is also used for firmware upgrade.

.. code-block:: c

    struct GenericCmdHeader {
        u8 magic_0x7E;
        u8 version;
        be12 transaction_id;
        be12 fragment_id;
        u8 flags;
        u8 service_id;
        u8 method_id;
        be16 error_code;
        be16 payload_size;
    };

Version
    Assumed to be there to support several versions of the same method call in
    upcoming firmware versions. Currently it is copied from query to reply.

Transaction and Fragment ID
    Used for commands that require payloads larger than the size of a 802.11n
    packet. Currently only seen in use for firmware upgrades. Set to 0 for most
    commands.

Flags
    Usually set to ``0x40`` in queries and set to ``((query.flags>>3)&0xFC|1)``
    in replies. If ``flags & 0x3``, this is a multi-fragment query.

Service and method ID
    Used to specify what method to call. Service ID very roughly groups similar
    methods together.

Error code
    Set in replies with an error code if the query failed.

Payload size
    Size of the query payload. Should be ``packet_size - sizeof
    (GenericCmdHeader) - sizeof (CmdHeader)``.

Service ID 0
~~~~~~~~~~~~

Contains mostly firmware related commands. For more information about firmware
update related commands, read the (non-existing yet) Firmware Update Commands
documentation.

+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| Method | Payload Size | Reply Size | Description                                                                                         |
+========+==============+============+=====================================================================================================+
| 0x00   | 0            | 8          | Get LVC firmware version. Returns two u32 values: running version and version installed.            |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x01   | 0            | ?          | Unknown.                                                                                            |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x02   | 4            | 0          | Start product update.                                                                               |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x03   | Max. 1028    | 4          | Contains update data. Input is struct { u32 block_idx; u8 data[]; }.                                |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x04   | 0            | 4          | End product update.                                                                                 |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x05   | 0            | 0          | Abort update.                                                                                       |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x09   | 1            | ?          | Returns hardcoded values.                                                                           |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x0a   | 1            | 4          | Gets version from EEPROM. Payload: 0 -> language_bank_and_ver, 1 -> tv_remocon, 2 ->                |
|        |              |            | service_firmware_version, 3 -> opening_screen_1, 4 -> opening_screen_2.                             |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x0b   | 9            | ?          | TVCode / Service / Screen 1 / Screen 2 start update.                                                |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x0c   | 0            | ?          | TVCode / Service / Screen 1 / Screen 2 end update.                                                  |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x0d   | 4            | ?          | Lang 1 / Lang 2 start update.                                                                       |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x0e   | 0            | ?          | Lang 1 / Lang 2 end update.                                                                         |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x0f   | 4            | ?          | Lang 1 / Lang 2 update language, bank and version.                                                  |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+

Service ID 1
~~~~~~~~~~~~

Wi-Fi keys related methods.

+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| Method | Payload Size | Reply Size | Description                                                                                         |
+========+==============+============+=====================================================================================================+
| 0x05   | 0            | 113        | Returns 13 bytes of WL_CONFIG_VARS, 1 byte of wifi_auth_mode, 1 byte of wifi_enc_type, 32 bytes of  |
|        |              |            | SSID, 1 byte of SSID length, 64 bytes of PSK, 1 byte of PSK length.                                 |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x08   | 0            | 16         | Returns the Wake On WLAN (WOWL) key.                                                                |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+

Service ID 2
~~~~~~~~~~~~

+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| Method | Payload Size | Reply Size | Description                                                                                         |
+========+==============+============+=====================================================================================================+
| 0x00   | 0            | 0          | Does not do anything, simply returns with error code 0.                                             |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x0c   | 0            | 272        | Returns 16 bytes of device ID and 256 bytes of device certificate.                                  |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+

Service ID 3
~~~~~~~~~~~~

Wi-Fi stats and signal related methods.

+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| Method | Payload Size | Reply Size | Description                                                                                         |
+========+==============+============+=====================================================================================================+
| 0x03   | 1            | 13         | Returns phy_stats from the Wi-Fi adapter.                                                           |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x04   | 1            | 152        | Returns pkt_stats from the Wi-Fi adapter.                                                           |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x05   | 1            | 448        | Returns wmm_stats from the Wi-Fi adapter.                                                           |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x06   | 0            | 2          | Returns RSSI from the Wi-Fi adapter (one byte for each of two antennas).                            |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+

Service ID 4
~~~~~~~~~~~~

+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| Method | Payload Size | Reply Size | Description                                                                                         |
+========+==============+============+=====================================================================================================+
| 0x01   | 1            | XXX        | Shuts down or reboots to a different mode. Payload 0 reboots to normal mode (if not already), 2     |
| 0x0e   |              |            | reboots to firmware update mode, 4 or 6 shuts down.                                                 |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x04   | 0            | 28         | Get version information.                                                                            |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x05   | 0            | 1          | Returns an integer indicating the Gamepad's current state (read from uic).                          |
|        |              |            | Returned value it retvals[current_state] with:                                                      |
|        |              |            | ``u8 retvals[] = {0, 10, 4, 5, 6, 1, 2, 3, 9, 7, 8, 11 };``                                         |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x08   | 4            | ?          | Unknown.                                                                                            |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x09   | 0            | 4          | Unknown.                                                                                            |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x0a   | 1            | ?          | Unknown.                                                                                            |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x0b   | 0            | 1          | Unknown.                                                                                            |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x14   | 4            | ?          | Unknown.                                                                                            |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x19   | 0            | 12         | Unknown.                                                                                            |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x1a   | 0            | XXX        | Shuts down the GamePad if flags = 0x42.                                                             |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+

Service ID 5
~~~~~~~~~~~~

Miscellaneous commands.

+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| Method | Payload Size | Reply Size | Description                                                                                         |
+========+==============+============+=====================================================================================================+
| 0x00   | 0            | ?          | Gets the 6 byte value from table @f8d0 in UIC (via UIC command 0x0a).                               |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x01   | 1            | 0          | Enables (payload 01) or disables (payload 00) the GamePad sensor bar via a GPIO.                    |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x02   | 102          | ?          | Sends 102 bytes to UIC via command 0x0c. Data format: u16, u16, { u8, u8, u8 }[32], u16.            |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x06   | 0            | 772        | Returns the 4 byte UIC firmware version followed by the first 768 bytes of the UIC EEPROM.          |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x08   | 1            | ?          | Unknown, sent when turning off the GamePad display (from the Home menu). Payload 00 disables video. |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x0a   | 1            | ?          | Unknown, sent when turning off the GamePad display (from the Home menu). Payload 00 disables sound. |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x0b   | 0            | 1          | Unknown, returns -1 or 0.                                                                           |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x0c   | 258          | ?          | Updates data from the UIC EEPROM. Payload format: u8 index, u8 length, u8 data_and_crc[].           |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x0e   | 5            | ?          | Unknown.                                                                                            |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x0f   | 0            | ?          | Unknown.                                                                                            |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x10   | 531          | ?          | Issues an ICTAG (NFC) command. See the (non-existing) NFC command documentation.                    |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x11   | Max. 520     | Max. 520   | Issues an IRCOM command. First byte of the payload: 0 -> connect, 1 -> send, 2 -> receive,          |
|        |              |            | 3 -> disconnect.                                                                                    |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x14   | 1            | 0          | Sets the LCD brightness, between 1 (minimum) and 5 (maximum, included).                             |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x15   | 0            | 1          | Returns the current LCD brightness, between 1 (minimum) and 5 (maximum, included).                  |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x16   | 1            | ?          | Unknown, IRCOM related. First byte of the payload should be either 0 or -1.                         |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x17   | 7            | ?          | Unknown, sends a TV command. First 5 bytes of the payload are the remocon ID.                       |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x18   | 10           | ?          | Sets the two remocon IDs (5 byte strings).                                                          |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x19   | 13           | ?          | Set LCD parameters via i2c to the LCD controller chip.                                              |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+
| 0x1a   | 0            | 13         | Get LCD parameters via i2c to the LCD controller chip.                                              |
+--------+--------------+------------+-----------------------------------------------------------------------------------------------------+

UVC/UAC command
---------------

For some reason, UVC/UAC commands are not using the generic command header but
have their own query type. UVC/UAC commands control settings of the internal
camera and microphone.

.. code-block:: c

    struct UvcUacCommand {
        u8 f1;
        u16 unknown_0;
        u8 f3;
        u8 mic_enable;
        u8 mic_mute;
        s16 mic_volume;
        s16 mic_volume_2;
        u8 unknown_A;
        u8 unknown_B;
        u16 mic_freq;
        u8 cam_enable;
        u8 cam_power;
        u8 cam_power_freq;
        u8 cam_auto_expo;
        u32 cam_expo_absolute;
        u16 cam_brightness;
        u16 cam_contrast;
        u16 cam_gain;
        u16 cam_hue;
        u16 cam_saturation;
        u16 cam_sharpness;
        u16 cam_gamma;
        u8 cam_key_frame;
        u8 cam_white_balance_auto;
        u32 cam_white_balance;
        u16 cam_multiplier;
        u16 cam_multiplier_limit;
    };

The meaning and values of most of these fields is not known yet.

This command is issued every second. Recent versions of the GamePad firmware
will only show the video stream after receiving one such command.

Setting f3 to 1 switches the GamePad to vWii mode, setting it to 3 shuts it down
(this is how the GamePad is shut down in vWii mode).

Time command
------------

Once again, this should have no reason to be its own query type. Time commands
are used to set time information on the GamePad. DRH usually sends a time
command when a DRC connects to it.

.. code-block:: c

    struct TimeCommand {
        u16 days_counter;
        u16 padding;
        u32 seconds_counter;
    };

This is counting from an unknown point in time.
