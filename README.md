# DRlib_ABUS_CFF3000
Arduino Library for controlling the ABUS Hometek Pro CFF3000 Keyfob controlling an ABUS remote doorlock

# WARNING
This project was written by an ape trying to learn to code. If you can improve it, you're very VERY welcome! Just contact me here on github and I'll figure out how to connect you to this project. Github is a first to me, too.

# About
This project started out as a learning piece, trying to understand how I can hack a device that is not meant to be used this way and control it using an Arduino. As such, this code started out messy and still is. I learned C++ while I wrote this, so please understand that this is not the most optimized piece of code on the web. It works, that's what is important to me.

# The Setup
So you decided to buy one of those new-fangled remote doorlock drives and it ended up being the ABUS Hometek Pro CFA3000 and you bought at least 2 matching Keyfobs, the ABUS CFF3000. And now you start wondering if you could remote control the remote control. This library enables you to do just this.

# You will need these devices:
- 1 ABUS Hometek Pro CFA3000 doorlock drive
- 1 ABUS Hometek Pro CFF3000 Keyfob, already paired with the doorlock drive
- 1 Arduino Uno, a Nano might work, alternatively, you might be able to adapt this library on a Raspberry Pi by changing some code.
- 1 Double-Relais Module or 2 Single Relais Modules (Default HIGH, can be changed in the library)
(Note: I used an Elegoo Uno and Elegoo Nano during development. Worked fine until I accidentally angered the angry pixies and they blew all of the magic smoke out of it.)

# Materials for hacking:
- Thin insulated wires, 0,1mm² is barely okay
- Soldering Iron with a thin tip and temperature control
- Thin solder or good soldering technique
- A flathead screwdriver for opening the case
- Good lighting
- Steady hands
- Nerves of Steel

# Materials for the Switch Simulator for testing the Example Code (optional):
- 1 Breadboard, small one is fine
- 3 pushbutton Switches
- 3 Pull-Up Resistors (I simply used regular 10k Resistors)
- A bunch of Jumper wires (about 14 should be enough)

# Hacking the Device
This part is crudely documented in the h-File of the library, a proper photo is needed or a good sketch.

# Wiring the Relais
TODO
...basically short out both sides of the devices switches through the relais. Be careful with Relais having Jumper modules - I smoked an Arduino Nano by accidentally shorting it out through this Jumper. Measuring continuity before applying voltage is actually *really* helpful, you know...

# Setting up the breadboard (optional)
TODO
...basically just build 3 Switches with Pull-Up Resistors and pass those to the Arduino. Use the Pins in the example if you do not want to mess around.

# Implementing and starting the Code
1. Drop DRlib_ABUS_CFF3000.cpp and DRlib_ABUS_CFF3000.h into a directory named "DRlib_ABUS_CFF3000".
2. Move this directory into your libraries-Folder, usually into Documents\Arduino\libraries or similar.
3. Create a directory for the example named "DRlib_ABUS_CFF3000_sw_example" and drop DRlib_ABUS_CFF3000_sw_example.ino into it
4. Open it with your Arduino Editor of choice - your procedure may differ from mine. That is fine. It's just an example.
5. Attach the library in Sketch -> Attach library. The library will appear here if you chose the correct library folder above.
6. Set your Arduino Device and COM-Port.
7. Compile and Run the Code
8. Test the buttons nearby the remote doorlock so you can close it if you pushed the open-button

# Further ideas
- Attach that Serial-Command based example you wrote.
- Add an "attach to new doorlock" serial function so the whole cable mess can stay in a box nearby the door.
- Figure out how to write more beautiful and shorter code.
- Shrink compilation size.
- Translate german parts of the code into english
