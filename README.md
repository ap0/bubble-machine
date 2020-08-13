# Bubble Machine

This is the design and code for my bubble machine.  You can find more info at the blog post here: https://medium.com/@adamargo/an-arduino-powered-3d-printed-bubble-machine-4cf9241d1459

## Notes

The parts were designed from scratch in Fusion 360.  The electronics were designed to use a 5V input supply so they could be powered by phone batteries.  At max settings, my power supply reported that it was consuming about 1.4A, so I'd be safe and round up to 2A for the output on the USB port.

The Arduino code runs the stepper motion in the event loop so minimize jitters when updating the LCD.  The I2C LCD is very slow to update.  A better version might figure out how to output directly without I2C.

## Parts List

To build this, you will need:

* at least 4 feet of 3/8" dowel rods ($1 at Home Depot)
* 4x M3x10mm screws and square nuts (I bought these nuts: https://www.amazon.com/Honbay-100pcs-Stainless-Steel-Square/dp/B06XPFLNBS -- they are a little thicker than the nuts that come on e.g. the Prusa).  These can be longer than 10mm if you don't have any; that's just the minimum.
* 4x M2.5x12mm M2 screws and nuts for the LCD -- these can be a little longer if you don't have 12mm -- up to 15mm should be fine, they'll just stick out inside the housing.
* Type 130 12V motor (I used the ones from this kit https://www.amazon.com/EUDAX-Electric-Magnetic-Connector-Propeller/dp/B07GDP2FCL -- the red housing seems to be the key, because they're a standard type 130 size motor, but lower voltage ones do not work and will burn up)
* Arduino Nano
  * I got one with pins soldered onto the board. This makes it easy to slot into the protoboard.
* 2 10k+Ω potentiometers (ones with a 7mm shaft diameter)
* A 16x2 I2C LCD module (I got these: https://www.amazon.com/GeeekPi-Character-Backlight-Raspberry-Electrical/dp/B07S7PJYM6) -- this part isn't required for the machine to function -- you could easily redesign the housing to not use it.
* 50x75mm protoboard
* 2x 2-pin terminal blocks
* ULN2003A Darlington array and socket
* IRF520 or RFP30N06LE MOSFET (it has to be a logic-level MOSFET, i.e. can be driven by 5V from Arduino)
* 2.54mm header pins and sockets
* 4 .1 μF capacitors
* 2 diodes
* A 5V 2.0A power supply (1.4A is the highest output I've seen on the meter -- the fan motor draws a lot of energy at high speeds) -- if you limited the MOSFET's output then you could probably live with 1A.  I'm using an Anker battery pack and it works fine.

## 3D printed parts

Total print time is around 24 hours at .15mm quality settings in PrusaSlicer for a MK3S.  You can't quite fit everything onto the bed at once.  The parts are designed to have a face to print on and should require minimal supports.  You will have to block the gap on the outer ring of the fan mount.

### Supports you should add

I tried to make it use minimal supports.  You should only need them on three of the parts:
* On the blower fan motor holder, you should add a support under the small area that's a clearance for the cover to sit flush, and potentially to hold up the other side of the neck.  I added a small slab in PrusaSlicer and it worked fine.
* On the two knobs you'll have to add supports to the bottom because it's got some inset holes to sit flush over the potentiometer nuts.

I printed mine in a few different filaments.  I used orange Prusament PETG, the gray Prusa (not Prusament) PLA that came with the MK3s, and yellow and blue Hatchbox PLA.  No real rhyme or reason to it other than aesthetics.  PLA works well for the housing as it's quite large.  The housing uses about 100g of filament and took about 11 hours to print.  I really like Hatchbox PLA -- the colors are very nice, it's not very expensive, and it prints great.  I also printed the knobs in the same color of the part they control, just as a nice touch.

The bubble wand hub is the most challenging part to print.  I found that using 0.2mm layer heights worked slightly better to deposit a little more filament on the first layer to get the ridges to stick.  My final print of it had only two of those instances not stick.  The quality and calibration of your printer will make or break this part.  