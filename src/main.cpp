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

#define SERVICE_UUID        "d5875408-fa51-4763-a75d-7d33cecebc31"
#define CHARACTERISTIC_UUID "a4f01d8c-a037-43b6-9050-1876a8c23584"

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

void print_esp_info(){
  double_t scale = 1024.0;
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  Serial.printf("Cpu   : (core) %d, (rev) %d, (freq) %d MHz\n", chip_info.cores, ESP.getChipRevision(), ESP.getCpuFreqMHz());
  Serial.printf("Build : (sdk) %s, (espidf) %s\n", ESP.getSdkVersion(), esp_get_idf_version());
  Serial.printf("SRam  : %6.1lf / %6.1lf KB\n", ESP.getFreeHeap()/scale, ESP.getHeapSize()/scale);
  Serial.printf("SPIRam: %6.1lf / %6.1lf KB\n", ESP.getFreePsram()/scale, ESP.getPsramSize()/scale);
  Serial.printf("Flash :          %6.1lf KB %5.1lf MHz\n", ESP.getFlashChipSize()/scale, ESP.getFlashChipSpeed()/1000000.0);
}

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

void send_ble(const struct quirc_data *data){
  if (!deviceConnected) {
    return;
  }
  char buffer[1024];
  sprintf(buffer, "%s", data->payload);
  pCharacteristic->setValue(buffer);
  pCharacteristic->notify();
  //pCharacteristic->indicate();
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
    send_ble(&data);
  }
}

void task_check_camera(void* param){
  Serial.println("task_check_camera start");
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

void app_ble_init() {
  BLEDevice::init("M5Camera QR Scanner");

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
}

void setup() {
  Serial.begin(115200);

  Serial.println("Camera init");
  if (ESP_OK != app_camera_init()) {
    Serial.println("Camera init failed");
    return;
  }

  Serial.println("BLE init");
  app_ble_init();

  print_esp_info();
  // start task of check qrcode
  xTaskCreatePinnedToCore(task_check_camera, "task_check_camera", 1024*50, NULL, 5, NULL, 1);
}

void loop() {
  delay(10000);
}
