# qrDoor - Scans QRCodes vie ESP32-CAM and send them as MQTT



qrCode is a generic application to read QRCodes with a ESP32-CAM module and forward them via MQTT.

It also supports, controllable by MQTT :

- a  triggered output port

- 2 LED status outputs

- 1 Input (e.g. for door bell)



## Setup

Install the Arduino IDE

add the ESP32 Library



1. goto “Menu Arduino IDE > Preferences > Settings > Additional board manager URLs”.
2. Add “[https://dl.espressif.com/dl/package_esp32_index.json”](https://dl.espressif.com/dl/package_esp32_index.json%E2%80%9D) and click OK.
3. Restart Arduino IDE and connect your board to a USB port.
4. Install the Esp32 library by going to “Tools > Board > Boards Manager > Search for Esp32 > Install Esp32 from Espressif Systems”.
5. Select the right board: “Tools > Board > ESP32 Arduino > AI Thinker ESP32-CAM”.
6. Select the right port by going to “Tools > Port” and then selecting your serial port ([it depends on your operating system](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/establish-serial-connection.html)).
