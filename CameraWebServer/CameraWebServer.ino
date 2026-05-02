#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <esp_wifi.h>

// ===========================
// Select camera model in board_config.h
// ===========================
#include "board_config.h"

// ===========================
// Direct ESP32-CAM <-> receiver Wi-Fi (no external router)
// ===========================
const char *apSsid = "ESP32CAM-LINK";
const char *apPassword = "camdisplay123";
constexpr uint8_t apChannel = 6;
constexpr uint8_t apMaxClients = 1;

void startCameraServer();
void setupLedFlash();

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  Serial.printf("Sensor PID: 0x%02X\n", s->id.PID);

  // Keep model-specific orientation fixups.
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);  // flip it back
  }

  // Global color tuning for the LCD receiver path.
  // Ranges (typical): brightness/contrast/saturation in [-2..2].
  s->set_brightness(s, 1);
  s->set_contrast(s, 1);
  s->set_saturation(s, 2);
  s->set_ae_level(s, 1);
  s->set_aec2(s, 1);
  s->set_awb_gain(s, 1);
  s->set_whitebal(s, 1);

  // Use a smaller camera frame to match the LCD pipeline and cut Wi-Fi bandwidth.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QQVGA);  // 160x120
    s->set_quality(s, 16);                 // lower bandwidth than default 10
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash();
#endif

  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);

  bool apOk;
  if (strlen(apPassword) == 0) {
    apOk = WiFi.softAP(apSsid, nullptr, apChannel, false, apMaxClients);
  } else {
    apOk = WiFi.softAP(apSsid, apPassword, apChannel, false, apMaxClients);
  }
  if (!apOk) {
    Serial.println("Failed to start camera SoftAP");
    return;
  }

  delay(200);
  IPAddress apIP = WiFi.softAPIP();
  wifi_config_t apCfg;
  memset(&apCfg, 0, sizeof(apCfg));
  esp_err_t cfgErr = esp_wifi_get_config(WIFI_IF_AP, &apCfg);
  Serial.println("Camera SoftAP started");
  Serial.print("SSID: ");
  Serial.println(apSsid);
  if (cfgErr == ESP_OK) {
    Serial.printf("AP channel: %u, auth: %u, max clients: %u\n",
                  apCfg.ap.channel,
                  apCfg.ap.authmode,
                  apCfg.ap.max_connection);
  } else {
    Serial.printf("AP config read failed: %d\n", (int)cfgErr);
  }
  Serial.print("AP IP: ");
  Serial.println(apIP);

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(apIP);
  Serial.println("' to connect");
}

void loop() {
  // Do nothing. Everything is done in another task by the web server
  delay(10000);
}
