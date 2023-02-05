/*
 * Required Libraries
 * WiFiManager Library
 * Arduino Json Library 5.xx !!
 * PubSub Client Library
 *     NOTE: Read README.md of how to tweak the library!!
 * 
 */

#include <string.h>
#include "quirc_internal.h"
#include "quirc.h"

 #if !defined ESP32
  #error This sketch is only for an ESP32Cam module
#endif

#include "esp_camera.h"       // https://github.com/espressif/esp32-camera

#include "config.h"


#include <WiFiMQTTManager.h>
#include <PubSubClient.h>

// ##########  Settings ########
  const int TimeBetweenStatus = 600;                     // speed of flashing system running ok status light (milliseconds)
  const int indicatorLED = 33;                           // onboard small LED pin (33)

// Button that will put device into Access Point mode to allow for re-entering WiFi and MQTT settings
#define RESET_BUTTON 1

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM


#include "camera_pins.h"

uint32_t lastStatus = millis();           // last time status light changed status (to flash all ok led)

WiFiMQTTManager wmm(RESET_BUTTON, AP_PASSWORD);  // AP_PASSWORD is defined in the secrets.h file

void setup_camera(){
    camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf((const char*)F("Camera init failed with error 0x%x\n"), err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif
}



static char* TAG="QR";
static const char *data_type_str(int dt) {
  switch (dt) {
  case QUIRC_DATA_TYPE_NUMERIC:
    return "NUMERIC";
  case QUIRC_DATA_TYPE_ALPHA:
    return "ALPHA";
  case QUIRC_DATA_TYPE_BYTE:
    return "BYTE";
  case QUIRC_DATA_TYPE_KANJI:
    return "KANJI";
  }

  return "unknown";
}

void dump_data(const struct quirc_data *data) {
  Serial.printf((const char*)F("    Version: %d\n"), data->version);
  Serial.printf((const char*)F("    ECC level: %c\n"), "MLHQ"[data->ecc_level]);
  Serial.printf((const char*)F("    Mask: %d\n"), data->mask);
  Serial.printf((const char*)F("    Data type: %d (%s)\n"), data->data_type,
      data_type_str(data->data_type));
  Serial.printf((const char*)F("    Length: %d\n"), data->payload_len);
  Serial.printf((const char*)F("    Payload: %s\n"), data->payload);

  if (data->eci)
    Serial.printf((const char*)F("    ECI: %d\n"), data->eci);
}

void qr_recognize(uint8_t *buffer, int width, int heigth) {
//  i++;
  Serial.println("geht 1");
  Serial.printf((const char*)F("begin to qr_recognize\r\n"));
  Serial.printf((const char*)F("width: %i height: %i \r\n"),width, heigth);

  struct quirc *q;
  struct quirc_data qd;
  uint8_t *image;
  q = quirc_new();
  if (!q) {
    Serial.printf((const char*)F("can't create quirc object\r\n"));
    return;
  }

  Serial.printf((const char*)F("begin to quirc_resize\r\n"));
  if (quirc_resize(q, width, heigth) < 0)
  {
    Serial.printf((const char*)F("quirc_resize err\r\n"));
    quirc_destroy(q);
    return ;
  }
  image = quirc_begin(q, NULL, NULL);

// ##################   ist das hier richtig mit dem buffer????

 
  memcpy(image, buffer, width * heigth);
  quirc_end(q);
  Serial.printf((const char*)F("quirc_end\r\n"));
  int id_count = quirc_count(q);
  Serial.printf((const char*)F("id_count: %d\r\n"),id_count);
  if (id_count == 0) {
    Serial.printf((const char*)F("Error: not a valid qrcode\n"));
    quirc_destroy(q);
    return ;
  }
  Serial.printf((const char*)F("id_count: %d\r\n"),id_count);
  
  struct quirc_code code;
  Serial.println("geht 2");
  quirc_extract(q, 0, &code);
  Serial.println("geht 3");
  quirc_decode(&code, &qd);
  Serial.println("geht 4");
  //return ;
  dump_data(&qd);
  //dump_info(q);
  quirc_destroy(q);
//  j++;
  Serial.printf((const char*)F("finish recoginize\r\n"));

}

void setup() {
  //Serial.begin(115200);
  Serial.println(F("WiFiMQTTManager Basic Example"));

  setup_camera();

  
  // set debug to true to get verbose logging
  // wm.wm.setDebugOutput(true);
  // most likely need to format FS but only on first use
  //wmm.formatFS = true;
  // optional - define the function that will subscribe to topics if needed
  //wmm.subscribeTo = subscribeTo;
  // required - allow WiFiMQTTManager to do it's setup
  wmm.setup(__SKETCH_NAME__);
  // optional - define a callback to handle incoming messages from MQTT
  //wmm.client->setCallback(subscriptionCallback);
  
}

void loop() {

  // required - allow WiFiMQTTManager to check for new MQTT messages, 
  // check for reset button push, and reconnect to MQTT if necessary 
  wmm.loop();

  camera_fb_t *fb = esp_camera_fb_get();

  if (fb){

  // for each pixel in image
  int width = fb->width;
  int height = fb->height;
  // just the first 3 lines
  // for (size_t i = 0; i < (width * 3); i++) {
  for (size_t i = 0; i < 0; i++) {
    const uint16_t x = i % width;                   // x position in image
    const uint16_t y = floor(i / width);            // y position in image
    byte pixel = fb->buf[i];                     // pixel value

    // show data
      if (x==0) Serial.println();           // new line
      Serial.print(String(pixel));                  // print byte as a string
      Serial.print(", ");

  } 
  
  qr_recognize(fb->buf,width,height);
  esp_camera_fb_return(fb);                      // return storage space
}else{
  Serial.println("Error: no  ");
}


  // optional - example of publishing to MQTT a sensor reading once a 1 minute
  long now = millis();
  if (now - wmm.lastMsg > 60000) {
    wmm.lastMsg = now;
    float temperature = 70; // read sensor here
    Serial.print("Temperature: ");
    Serial.println(temperature);
    char topic[100];
    snprintf(topic, sizeof(topic), "%s%s%s", "sensor/", wmm.deviceId, "/temperature");
    wmm.client->publish(topic, String(temperature).c_str(), true);
  }
  // flash status LED to show sketch is running ok
    if ((unsigned long)(millis() - lastStatus) >= TimeBetweenStatus) { 
      lastStatus = millis();                                               // reset timer
      digitalWrite(indicatorLED,!digitalRead(indicatorLED));               // flip indicator led status
    }
 
}



// optional function to subscribe to MQTT topics
void subscribeTo() {
  Serial.println("subscribing to some topics...");  
  // subscribe to some topic(s)
  char topic[100];
  snprintf(topic, sizeof(topic), "%s%s%s", "switch/", wmm.deviceId, "/led1/output");
  wmm.client->subscribe(topic);
}

// optional function to process MQTT subscribed to topics coming in
void subscriptionCallback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  //if (String(topic) == "switch/esp1234/led1/output") {
  //  Serial.print("Changing led1 output to ");
  //}


  
}
