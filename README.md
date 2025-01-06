# Data Interface for Clockwise Tools Caliper Gauges

This project is a reverse engineered version of the Clockwise Tools DTCR-02 RS232 Data Cable for Clockwise tools DCLR-XX05 calipers.
And given the similarity to [liba2k's VINCA Reader](https://github.com/liba2k/VINCA_reader) it seems likely that this data interface will work with VINCA series calipers as well.

The DTCR-02 Data Cable allows DCLR-XX05 series calipers to transfer their current measurement to a connected computer by emulating a USB keyboard.
This data interface aims to improve on this by adding additional features, and providing a basis for using the calipers serial protocol for other use cases.

Some of the data interface features include:
- Emulates a generic USB keyboard, compatible with Linux, Windows, MacOS, iOS[^ios_keyboard], Android[^android_keyboard].
- Supports multiple ways to trigger sending the measurement to the computer.
    - Included push button on the data interface.
    - 2 pin JST connector for a remote push button or foot pedal.
    - A custom USB cable with a push button mounted on the calipers. (See schematics for details)
- Two LEDs to indicate power status, and the successful reception of data packets from the calipers.
- Multiple different configuration options set via a DIP switch.
    - The option to send `CTRL + A` before the measurement is sent so previous measurements are overwritten.
    - Enable sending the units (mm/in) after the measurement. (Such as `1.23 mm`)
    - Setting any combination of a newline, tab, comma, or space to be sent after the measurement.
    - Enabling a buzzer to provide an audible queue when a measurement is sent.
    - 


## The "RS232" Serial Protocol
Clockwise Tools own online store page for the Data Cable, as well as other online marketplaces use the term "RS232" as part of the product title.
However, upon investigating the data being sent over the micro USB B port, it is instead using synchronous serial communication over the D+ and D- wires of the USB port.
This is effectively SPI, just without the chip select and MISO signals.

The calipers transmit 24 bits of data every ~150ms over the D+ and D- wires at 1.5V.
It seems to transmit regardless of if anything is plugged in to the USB port, or the calipers are turned on or off.
During data transmission, the clock has a rate of ~0.4ms per clock cycle.
The D+ wire carries the data signal, while D- carries the clock.
The USB ports +5V pin is also powered by the calipers, however only at 1.5V.

> [!NOTE]
> The calipers exhibit weird behavior when too much current is drawn over the USB port.
> This can cause measurements to fluctuate significantly, or the screen to dim.
> Anything connected to the calipers USB port must not draw over **~0.3mA** of current in total from all and any of its pins.
> This means that the parallel resistance between VBUS, D+, and D- to GND of the USB connector must be at least ~5kΩ, but preferably well above that.

As far as I can tell, there is no way to communicate from the device to the calipers, as they are constantly transmitting on D+ and D-, and due to the above, pulling VBUS low causes the calipers to lose power.

One 24 bit packet consists of 4 bits for flags, and 20 bits for the measurement data.
The full details are laid out in [Harbor Freight Caliper Data Format](https://www.yuriystoys.com/2013/07/chinese-caliper-data-format.html).
Essentially, there are two flags, one for the unit (mm/in) and one for the sign (+/-).
The measurement data is fixed point, and its scale depends on the unit selected.
The calipers will send whatever unit is currently selected on the display.
Note that the fraction mode is sent exactly the same as inches over the USB port.

The full implementation of decoding and converting the 24 bit packets can be seen in ClockwiseCaliper.cpp.

## Hardware
Schematics for a working implementation of the data interface are included with this project.
I built my own out of parts that I had lying around, however there are some improvements that can be made given the right parts.


[^ios_keyboard]: iOS devices will complain if the device reports that it uses more than 100mA.
    By default, most Arduino's report their bMaxPower USB configuration descriptor to be 500mA.
    To change the bMaxPower descriptor you have to change the line `#define USB_CONFIG_POWER (500)` to be 100 or less in `USBCore.h` of the Arduino core.
    \
    Note that for iDevices with a lightning port, a Lightning to USB Camera Adapter (only the USB port) will allow connecting the device, so long as it reports a lower power consumption.
    Additionally, the Lightning to USB 3 Camera Adapter (both a USB port and a lightning port) MAY allow devices that report more than 100mA to work, so long as a charging cable is plugged in.
    
[^android_keyboard]: I have not tested any Android devices with the data interface, but I would assume that with the right adapter, and potentially the iOS bMaxPower tweak above that it would work.