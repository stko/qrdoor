/*
 * wifiMQTTManager from https://github.com/dreed47/WifiMQTTManager/blob/master/examples/Basic/Basic.ino
 * Required Libraries
 * WiFiManager Library
 * Arduino Json Library 5.xx !!
 * PubSub Client Library
 *     NOTE: Read README.md of how to tweak the library!!
 * 
 * Hardware connections (inspired from https://www.electroniclinic.com/esp32-cam-smart-iot-bell-circuit-diagram-and-programming/)
 * 
 * Pin 1 : 5V
 * Pin 2 : GND
 * Pin 3 : Door lock relay driver output 3.3V High active (GPIO 12)
 * Pin 4 : Door bell button switch to gnd (GPIO 13)
 * Pin 5 : Indicator LED output High active (GPIO 15)
 *
 */

#include <string.h>

#if !defined ESP32
#error This sketch is only for an ESP32Cam module
#endif

#include "qrtask.h"

#include "config.h"

#include "WiFiMQTTManager.h"
#include "PubSubClient.h"

// ##########  Settings ########
const int TimeBetweenStatus = 600; // speed of flashing system running ok status light (milliseconds)
const int indicatorLED = 33;       // onboard small LED pin (33)

// #define LED_PIN 4
#define LED_PIN indicatorLED

// Button that will put device into Access Point mode to allow for re-entering WiFi and MQTT settings
#define RESET_BUTTON 3

uint32_t lastStatus = millis(); // last time status light changed status (to flash all ok led)

extern QueueHandle_t xQrCodeQueue;
extern WiFiManager *wm;

WiFiMQTTManager wmm(RESET_BUTTON, AP_PASSWORD); // AP_PASSWORD is defined in the secrets.h file

TaskHandle_t QrReaderTask;

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  // Serial.println(F("WiFiMQTTManager Basic Example"));
  Serial.println("WiFiMQTTManager Basic Example");

  // set debug to true to get verbose logging
  wm->setDebugOutput(true);
  wm->setConnectTimeout(5);
  // most likely need to format FS but only on first use
  // wmm.formatFS = true;
  // optional - define the function that will subscribe to topics if needed
  wmm.subscribeTo = subscribeTo;
  // required - allow WiFiMQTTManager to do it's setup
  wmm.setup(__SKETCH_NAME__);
  // optional - define a callback to handle incoming messages from MQTT
  wmm.client->setCallback(subscriptionCallback);

  // Set up Core 1 task handler

  /* Create a queue capable of containing 10 pointers to qrcode string.  These are to be queued by pointers as they are
  relatively large structures. */
  xQrCodeQueue = xQueueCreate(10, sizeof(char *));

  if (xQrCodeQueue != NULL)
  {

    // xTaskCreate(qrtask, "qrtask", 32768, NULL, 1, NULL);
    xTaskCreatePinnedToCore(
        qrtask,
        "qrtask",
        32768,
        (void *)xQrCodeQueue,
        1,
        &QrReaderTask,
        1);
  }
}

void loop()
{
  // delay(500);
  // digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  //  Serial.println("Wloop");
  //   required - allow WiFiMQTTManager to check for new MQTT messages,
  //   check for reset button push, and reconnect to MQTT if necessary
  wmm.loop();
  char *qrCodeStr;
  while (xQueueReceive(xQrCodeQueue,
                       &qrCodeStr,
                       (TickType_t)0) == pdPASS)
  {
    Serial.print("received QR: ");
    Serial.println(qrCodeStr);
    char topic[100];
    snprintf(topic, sizeof(topic), "%s%s%s", "qrdoor/", wmm.deviceId, "/qrcode");
    wmm.client->publish(topic, qrCodeStr, true);
    free(qrCodeStr);
  }

  // optional - example of publishing to MQTT a sensor reading once a 1 minute
  long now = millis();
  if (now - wmm.lastMsg > 60000)
  {
    wmm.lastMsg = now;
    float temperature = 70; // read sensor here
    Serial.print("Temperature: ");
    Serial.println(temperature);
    char topic[100];
    snprintf(topic, sizeof(topic), "%s%s%s", "sensor/", wmm.deviceId, "/temperature");
    wmm.client->publish(topic, String(temperature).c_str(), true);
  }
  // flash status LED to show sketch is running ok
  if ((unsigned long)(millis() - lastStatus) >= TimeBetweenStatus)
  {
    lastStatus = millis();                                  // reset timer
    digitalWrite(indicatorLED, !digitalRead(indicatorLED)); // flip indicator led status
  }
}

// optional function to subscribe to MQTT topics
void subscribeTo()
{
  Serial.println("subscribing to some topics...");
  // subscribe to some topic(s)
  char topic[100];
  snprintf(topic, sizeof(topic), "%s%s%s", "switch/", wmm.deviceId, "/led1/output");
  wmm.client->subscribe(topic);
}

// optional function to process MQTT subscribed to topics coming in
void subscriptionCallback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // if (String(topic) == "switch/esp1234/led1/output") {
  //   Serial.print("Changing led1 output to ");
  // }
}
