# QRDoor - A QRCode Reader & MQTT Door Opener

Even as made as add-on for the [Zuul Acess Control System](https://github.com/stko/zuul-ac), it's also suitable whereever a MQTT driven QR code reader and door opener output is needed.

## What it does
* it quickly reads all codes which are presented to the camera
* the read codes are sent as a mqtt message
* it waits for incoming mqtt messages to activate the output pin (to control a door opener)
* as a convinience function, it monitors an input pin (as door bell button) and reports its state change via mqtt

## Setup
For the hardware connect your rpi pico as shown in the circuit/qrcode_pico schematic.

For the software installion the Thonny IDE was used. The steps are
* for micropython follow the instructions on https://micropython.org/download/RPI_PICO_W/. Make sure you install the version for the *WIFI version* of the Rasperry Pico!
* connect your Thonny IDE with your pico device
* In Thonny, in "Tools/Manage Packages.." install the both packages umqtt.simple2 and umqtt.robust2 (MAKE SURE YOU GOT THE ....2 VERSION)
* copy the provided file `settings_template.json`to `settings.json`
* edit the new file `settings.json` and set your own settings
* via Thonny, copy both main.py and settings.json onto your pico device
* run the program

## Operation
* at start qrdoor tries to connect with the given WIFI. This loop is repeated until the connection is made.
* then it waits for either for a scanned code, a button press or an open command

## MQTT commands
### Scanned code
When a code is read, its published to `qrdoor/<device- id>/code`
  
### Button Press
When the button is pressed or released, the state is reported to `qrdoor/<device- id>/button` as `on` or `off`

### Door Open command
When the `qrdoor/<device- id>/open` \<secs\> topic is received, both LED and output pins are set to High for \<secs\> seconds. To avoid unwanted longer power output, this time is limited to 5 secs.



## History
Originally the whole project was started to use a ESP32CAM module for it. This work can be found in the qrdoor folder as an Arduino Project. It finally worked in some way, but it came out that the ESPCAM can't handle it properly when suddenly a mobile phone with a qrcode on it is presented to the camera. All the brightness control and auto focus need to be completely re- adjusted, which takes at least 10 seconds, and during the whole time the user needs to hold the mobile phone stable and calm in a constant position and distance in front to the camera - and this just don't work.

So finally the whole project was restarted, this time with a Raspberry Pico, Micropython and the ready-to-go QR-Code Reader from Waveshare (which seems to be a GROW GM861, which can be ordered in Asia). With that it just took a relaxed weekend to get it to work, mainly because of the very well working barcode reader.


## Links

Micropython Image *with* WLAN - Support:  https://micropython.org/download/rp2-pico-w/rp2-pico-w-latest.uf2
The QR Reader Module: https://www.waveshare.com/wiki/Barcode_Scanner_Module_(D)