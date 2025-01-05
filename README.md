# Data Interface for Clockwise Tools Caliper Gauges

This project is a reverse engineered version of the Clockwise Tools DTCR-02 RS232 Data Cable for Clockwise tools DCLR-XX05 calipers. And given the similarity to [liba2k's VINCA Reader](https://github.com/liba2k/VINCA_reader) it seems likely that this data interface will work with VINCA series calipers as well.

The Data Cable allows DCLR-XX05 series calipers to transfer their current measurement to a connected computer by emulating a USB keyboard. This project aims to improve on this by adding additional features, and providing a basis for using the calipers serial protocol for other use cases.

## The "RS232" Serial Protocol
As stated by Clockwise Tools own online store page for the Data Cable, it allegedly uses RS232 to communicate with the calipers. However, upon investigating the data being sent over the micro USB B port, it is instead using synchronous serial communication over the D+ and D- wires of the USB port.

The calipers transmit 24 bits of data every ~150ms over the D+ and D- wires at 1.5v. During data transmission, the clock has a rate of ~2ms per clock cycle . The D+ wire carries the data signal, while D- carries the clock. The USB ports +5v pin is also powered by the calipers, however only at 1.5v. As far as I can tell, the 1.5v is not used for data communication. 

> [!WARNING]
> The calipers exhibit weird behavior when too much current is drawn over the USB port. Causing measurements to fluctuate significantly, or the screen to dim. Anything connected to the calipers USB port must not draw over **~0.3ma** of current (~5kΩ @ 1.5v) in total from all and any of its pins.

One 24 bit packet consists of 4 bits for flags, and 20 bits for the measurement data. The full details are laid out in [Harbor Freight Caliper Data Format](https://www.yuriystoys.com/2013/07/chinese-caliper-data-format.html). Essentially, there are two flags, one for the unit (mm/in) and one for the sign (+/-). The measurement data is fixed point, and its scale depends on the unit selected.
The calipers will send whatever unit is currently selected on the display. Note that the fraction mode is sent exactly the same as inches over the USB port.

The full implementation of decoding and converting the 24 bit packets can be seen in ClockwiseCaliper.cpp.

## Hardware
Schematics for a working implementation of the data interface are included with this project. I built my own out of parts that I had lying around, however there are some improvements that can be made given the right parts.