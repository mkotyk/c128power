c128power
=========

Commodore 128 Power Helper

The aim of this project is to replace the orignal power supply
on a Commodore 128 with a modern ATX power supply.

The project makes use of an ATTiny85 for several features:

- Power on/off with the RESTORE key
- Generate Time Of Day clock (60Hz)
- Ability to switch Kernel ROM banks if modded for JiffyDOS
- Connected via serial user port to power off and switch ROM banks
  and any other cool features that will fit in 8K.


Caveats
-------

This modification, while not overly difficult, is not for everyone.
It requires a good working understanding of eletronics, and skill 
with soldering.  It also requires the ability to flash the ATTiny85 
micro controller and build a simple circuit.

Also, by switching to an ATX power supply, we must live without the 9VAC
power that was orignally supplied.  This impacts the following areas:

- User port won't have 9VAC to offer any periphials, so some devices
  like modems may not work
- Tape drive motor won't have power
- Time of day clock won't receive 60Hz from AC power, so it must be 
  simulated


Hardware
--------

Check under the hardware folder for instructions, schematics and photos.  
I've only tried this on my C=128, but might also add it to one of my C=64's.  
The same code and circuit should work on both.


Software
--------

This project uses the software serial routines from 
https://github.com/blalor/avr-softuart

The code has a Makefile and should compile under most versions of Linux with 
the avr-gcc tool chain and avrdude installed.  It also compiled under Atmel
Studio 6.2 for me.

WARNING: This project uses the /RESET pin, so once it's programmed via ISP,
you need to high-voltage clear the fuse to reprogram the device.
