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

other required Libraries

* WiFiManager Library 
* Arduino Json Library **5.xx** !!
* PubSub Client Library
* WiFiMQTTManager Library

## Hacks

After installing WiFiMQTTManager, go into the library file `.../WiFiMQTTManager.cpp` and move the `Serial.begin(115200);`line as the first line of the constructor

`WiFiMQTTManager::WiFiMQTTManager(int resetPin, char* APpassword) {
  Serial.begin(115200);
  wm = new WiFiManager;
  lastMsg = 0;
  formatFS = false;`

add the line `client->setBufferSize(512);` after the `client.reset(new PubSubClient(_espClient));`

`  client.reset(new PubSubClient(_espClient));
  client->setBufferSize(512);
  client->setServer(_mqtt_server, port);`

we modify the `SPIFFS.begin()` line from

` if (SPIFFS.begin()) {`

to 

 `if (SPIFFS.begin(false) || SPIFFS.begin(true)){ `

to format the filesystem at the inital start

In Tools-Board select :

- ESP32Wrover Module

- Speed 921600

- Flash Frequency 80Mhz

- Flash Mode QIO

- Partition Scheme: Huge app

in \~/snap/arduino/85/.arduino15/packages/esp32/hardware/esp32/1.0.6/cores/esp32/main.cpp

increase the task stack size

```cpp
#ifndef CONFIG_ARDUINO_LOOP_STACK_SIZE
#define CONFIG_ARDUINO_LOOP_STACK_SIZE 8192
#endif

#### ---- add this -----
#define CONFIG_ARDUINO_LOOP_STACK_SIZE 32768

## Credits

```

- The program skeleton comes the demo from WiFiMQTTManager

- The image capture routines are from [GitHub - donny681/ESP32_CAMERA_QR](https://github.com/donny681/ESP32_CAMERA_QR)

-  [GitHub - dlbeer/quirc: QR decoder library](https://github.com/dlbeer/quirc)

- qrcode code sample [GitHub - donny681/ESP32_CAMERA_QR](https://github.com/donny681/ESP32_CAMERA_QR)
