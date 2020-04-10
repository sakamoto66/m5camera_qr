#include <Arduino.h>
#include "BLEDevice.h"
#include "BLE2902.h"
#include "quirc_internal.h"
#include "esp_camera.h"
#include "qr_recognize.h"

//M5STACK_CAM PIN Map
#define CAM_PIN_RESET   15 //software reset will be performed
#define CAM_PIN_XCLK    27
#define CAM_PIN_SIOD    22
#define CAM_PIN_SIOC    23

#define CAM_PIN_D7      19
#define CAM_PIN_D6      36
#define CAM_PIN_D5      18
#define CAM_PIN_D4      39
#define CAM_PIN_D3      5
#define CAM_PIN_D2      34
#define CAM_PIN_D1      35
#define CAM_PIN_D0      32

#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    26
#define CAM_PIN_PCLK    21

#define CAM_XCLK_FREQ   20000000

#define SERVICE_UUID        "D5875408-FA51-4763-A75D-7D33CECEBC31"
#define CHARACTERISTIC_UUID "A4F01D8C-A037-43B6-9050-1876A8C23584"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
uint8_t value = 0;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

esp_err_t app_camera_init(){
  camera_config_t config = {};

  config.pin_reset = CAM_PIN_RESET;
  config.pin_xclk = CAM_PIN_XCLK;
  config.pin_sscb_sda = CAM_PIN_SIOD;
  config.pin_sscb_scl = CAM_PIN_SIOC;

  config.pin_d7 = CAM_PIN_D7;
  config.pin_d6 = CAM_PIN_D6;
  config.pin_d5 = CAM_PIN_D5;
  config.pin_d4 = CAM_PIN_D4;
  config.pin_d3 = CAM_PIN_D3;
  config.pin_d2 = CAM_PIN_D2;
  config.pin_d1 = CAM_PIN_D1;
  config.pin_d0 = CAM_PIN_D0;
  config.pin_vsync = CAM_PIN_VSYNC;
  config.pin_href = CAM_PIN_HREF;
  config.pin_pclk = CAM_PIN_PCLK;

  //XCLK 20MHz or 10MHz
  config.xclk_freq_hz = CAM_XCLK_FREQ;
  config.ledc_timer = LEDC_TIMER_0;
  config.ledc_channel = LEDC_CHANNEL_0;

  config.pixel_format = PIXFORMAT_GRAYSCALE;//YUV422,GRAYSCALE,RGB565,JPEG
  config.frame_size = FRAMESIZE_QVGA;//QQVGA-UXGA Do not use sizes above QVGA when not JPEG

  config.jpeg_quality = 20; //0-63 lower number means higher quality
  config.fb_count = 1; //if more than one, i2s runs in continuous mode. Use only with JPEG

  return esp_camera_init(&config);
}

void on_accept_qrcode(int num, const struct quirc_code *qcode){
  struct quirc_data data;
  quirc_decode_error_t err;

  err = quirc_decode(qcode, &data);

  if(err) {
    Serial.printf("Decoding Failed: %s", quirc_strerror(err));
    Serial.println();
  } else {
    dump_qrcode_info(&data);
  }
}

void task_check_camera(void* param){
  while(true){
    camera_fb_t* fb = esp_camera_fb_get();
    if(fb) {
      qr_recoginze(fb, on_accept_qrcode);
      esp_camera_fb_return(fb);
    } else {
      Serial.println("Camera capture failed");
    }
    delay(1000);
  }
}

void test() {
  Serial.println("A1");
  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  Serial.println("A2");
  pServer->setCallbacks(new MyServerCallbacks());

  Serial.println("B");
  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  Serial.println("C");
  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  Serial.println("D");
  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  Serial.println("E");
  // Start the service
  pService->start();

  Serial.println("F");
  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void setup() {
  Serial.begin(115200);
  BLEDevice::init("M5Camera QR Scanner");
  test();
  /*Serial.println("Camera init");
  if (ESP_OK != app_camera_init()) {
    Serial.println("Camera init failed");
    return;
  }

  // start task of check qrcode
  xTaskCreatePinnedToCore(task_check_camera, "task_check_camera", 102400, NULL, 5, NULL, 1);
  //*/
}

void loop() {
  if (deviceConnected) {
    Serial.printf("*** NOTIFY: %d ***\n", value);
    char buffer[10];
    sprintf(buffer, "{\"val\":%d}", value);
    Serial.printf(buffer);
    pCharacteristic->setValue(buffer);
    pCharacteristic->notify();
    //pCharacteristic->indicate();
    value++;
  }
  delay(2000);
}
