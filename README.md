TODO: Images

# Data Interface for Clockwise Tools Caliper Gauges

This project is a reverse engineered version of the Clockwise Tools DTCR-02 RS232 Data Cable for Clockwise tools DCLR-XX05 calipers.
And given the similarity to [liba2k's VINCA Reader](https://github.com/liba2k/VINCA_reader) it seems likely that this Data Interface will work with VINCA series calipers as well.

The DTCR-02 Data Cable allows DCLR-XX05 series calipers to transfer their current measurement to a connected computer by emulating a USB keyboard.
This Data Interface aims to improve on this by adding additional features, and providing a basis for using the calipers serial protocol for other use cases.

The Data Interface captures the most recent measurement from the calipers.
Then, when a button is pressed, the measurement is typed on the connected computer.
The measurement could be typed into any program, such as an Excel spreadsheet, or a 3D modeling program such as Blender.
Using the additional configuration options, the Data Interface can automatically advance to the next cell of a spread sheet, by either pressing `TAB` or `ENTER` after typing the measurement.

Some of the Data Interface features include:

- Emulates a generic USB keyboard, compatible with Linux, Windows, MacOS, [iOS*](#ios-devices), [Android*](#android-devices), and really anything that supports USB keyboards.
- Supports multiple ways to trigger sending the measurement to the computer.
  - Included push button on the Data Interface.
  - 2 pin JST connector for a remote push button or foot pedal.
  - A custom USB cable with a push button mounted on the calipers. (See schematics for details)
- Two LEDs to indicate power status, and the successful reception of data packets from the calipers.
- Multiple different configuration options can be set via a DIP switch.
  - The option to type `CTRL + A` before the measurement is type so previous measurements are overwritten.
  - Enable typing the units (mm/in) after the measurement. (Such as `1.23mm`)
  - Setting any of a newline, tab, comma, or space to be typed after the measurement.
  - Enabling a buzzer to provide audible feedback when a measurement is sent.
  - Enable the trigger input using the USB ports VBUS pin. (See schematics for details)

## Hardware

Really any Arduino should be able to read the data stream from the calipers, as the clock rate is relatively slow.
However, in order to emulate a USB keyboard, the Arduino must support it's own USB communication, or you must use something like [HoodLoader2](https://github.com/NicoHood/HoodLoader2).

Otherwise, there are no specific requirement as far as specialized components.
So long as it is close to the values in the schematic, it should work.

As part of the KiCad schematic, there is an example through-hole proto-board layout for double sided proto-boards. If you intend to make a surface mount version of this, you probably should not base it off of this.

## Installing to an Arduino

## Extra Details

### The "RS232" Serial Protocol

Clockwise Tools own online store page for the Data Cable, as well as other online marketplaces use the term "RS232" as part of the product title.
However, upon investigating the data being sent over the micro USB B port, it is instead using synchronous serial communication over the D+ and D- wires of the USB port.
This is effectively SPI, just without the chip select and MISO signals.

The calipers transmit 24 bits of data every ~150ms over the D+ and D- wires at 1.5V.
It seems to transmit regardless of if anything is plugged in to the USB port, or if the calipers are turned on or off.
The clock has a rate of ~0.4ms per clock cycle, and the clock phase and polarity are equivalent to SPI Mode 3.
However, because of the transistors used for level shifting, the polarity is inverted, so SPI Mode 1 is used.
The D+ wire carries the data signal, while D- carries the clock.
The USB ports +5V pin is also powered by the calipers, however only at 1.5V.

> [!NOTE]
> The calipers exhibit weird behavior when too much current is drawn over the USB port.
> This can cause measurements to fluctuate significantly, or the screen to dim.
> Anything connected to the calipers USB port must not draw over **~0.3mA** of current in total from all and any of its pins.
> This means that the parallel resistance between VBUS, D+, and D- to GND of the USB connector must be at least ~5kΩ, but preferably well above that.

As far as I can tell, there is no way to communicate from the device to the calipers, as they are constantly transmitting on D+ and D-, and due to the above, pulling VBUS low causes the calipers to lose power.

One 24 bit packet consists of 4 bits for flags, and 20 bits for the measurement data.
The full details are laid out in this article: [Harbor Freight Caliper Data Format](https://www.yuriystoys.com/2013/07/chinese-caliper-data-format.html).
Essentially, there are two flags, one for the unit (mm/in) and one for the sign (+/-).
The other two flags are either not used, or their meanings are unknown.
The measurement data is fixed point, and its scale depends on the unit selected.
The calipers will send whatever unit is currently selected on the display.
Note that the fraction mode is sent exactly the same as inches over the USB port.

The full implementation of decoding and converting the 24 bit packets can be seen in ClockwiseCaliper.cpp.

### Potential Improvements

I designed the schematics and built the device based off of parts that I had lying around.
Mostly from used parts taken from laser printers, coffee makers, and power supplies.
So, given access to the right parts, there are some improvements that can be made to the design.

- The three transistors used to do the voltage level shifting (**Q1**, **Q2**, and **Q3** in the schematic) should be replaced by MOSFETs.
Additionally, the resistors for those transistors should be removed, and pull-down resistors for the MOSFETs should be added, with values around 100kΩ. This is because of the maximum current requirement as outlined above.
- The program could technically use hardware SPI, however, there is an issue that occurs if the data stream get out of sync on the first byte, as the program is unable to detect the timing discrepancy.
This could be solved by controlling the SPI slave select input using another output pin.
Then activating it on the initial clock pluse, and deactivating it on the last clock pulse.
However, ensure that the slave select pin is accessible, as Arduino Pro Micro's use that pin for an onboard LED.

## Quirks

### iOS Devices

iOS devices will complain if the device reports that it draws more than 100mA over the USB port.
By default, most Arduino's report their bMaxPower USB configuration descriptor to be 500mA.

To change the bMaxPower descriptor you have to change the line `#define USB_CONFIG_POWER (500)` to be 100 or less in `USBCore.h` of the Arduino core.

Note that for iDevices with a lightning port, a Lightning to USB Camera Adapter (the one with only a USB port) will allow connecting the device, so long as it reports a lower power consumption.

Instead, I have heard that the Lightning to USB 3 Camera Adapter (the one with both a USB port and a lightning port) MAY allow devices that report more than 100mA to work, so long as a charging cable is plugged in.

### Android Devices

I have not tested any Android devices with the Data Interface, but I would assume that with the right adapter, and potentially the iOS bMaxPower tweak above that it would work.
