GPIO Controller
===============

The DRC SoC contains a GPIO controller that is used to control 8 external GPIO
pins (of which 6 are used in the current firmware).

Registers
---------

The following MMIO registers are used for GPIOs:

+----------+-------+--------------------------------------------------------------+
| Address  | Name  | Connected to                                                 |
+==========+=======+==============================================================+
| F0005100 | GPIO0 | Unknown, used in the LCD driver.                             |
+----------+-------+--------------------------------------------------------------+
| F0005108 | GPIO1 | Unknown, unused in the LVC firmware.                         |
+----------+-------+--------------------------------------------------------------+
| F000510C | GPIO2 | Unknown, used in the NFC driver.                             |
+----------+-------+--------------------------------------------------------------+
| F0005110 | GPIO3 | Unknown, called ``TRM_ENABLE_POWER_GPIO`` in the NFC driver. |
+----------+-------+--------------------------------------------------------------+
| F0005114 | GPIO4 | Controls the rumble motor.                                   |
+----------+-------+--------------------------------------------------------------+
| F0005118 | GPIO5 | Controls the sensor bar power.                               |
+----------+-------+--------------------------------------------------------------+
| F000511C | GPIO6 | Unknown, used in the CMOS driver.                            |
+----------+-------+--------------------------------------------------------------+
| F0005098 | GPIO7 | Unknown, unused in the LVC firmware.                         |
+----------+-------+--------------------------------------------------------------+

Configuring a GPIO
------------------

To configure a GPIO, write a combination of the following flags to the GPIO
register:

Direction
    * Output: ``0x200``
    * Input: ``0x800``

Unknown flags
    * ``0x1``
    * ``0x1000``
    * ``0x2000`` (only set when ``0x1000`` is also set)
    * ``0x4000``
    * ``0x8000``

For example, to configure the rumble motor GPIO as an output GPIO with pull-up
resistors::

    write32(MMIO_GPIO4, 0x8200);

To configure it as an input GPIO with pull-up resistors::

    write32(MMIO_GPIO4, 0x8800);

Setting or reading a GPIO
-------------------------

To set an output GPIO to 0/1, use the ``0x100`` bit::

    // 0
    write32(MMIO_GPIO4, read32(MMIO_GPIO4) & ~0x100);
    // 1
    write32(MMIO_GPIO4, read32(MMIO_GPIO4) | 0x100);

To read an input GPIO, use the ``0x400`` bit::

    int x = read32(MMIO_GPIO4) & 0x400;
