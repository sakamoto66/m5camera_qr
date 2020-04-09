#include <Arduino.h>
#include "BluetoothSerial.h"
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

BluetoothSerial SerialBT;

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

void setup() {
  SerialBT.begin("M5Camera QR Scanner");
  Serial.begin(115200);
  Serial.println("Camera init");
  if (ESP_OK != app_camera_init()) {
    Serial.println("Camera init failed");
    return;
  }

  // start task of check qrcode
  xTaskCreatePinnedToCore(task_check_camera, "task_check_camera", 102400, NULL, 5, NULL, 1);
}

void loop() {
  if(SerialBT.connected()){
    SerialBT.println("Hello World");
  }
  delay(1000);
}
