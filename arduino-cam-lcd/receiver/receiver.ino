#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <esp_wifi.h>

// TFT_eSPI self-contained config (avoid editing library User_Setup.h)
#define USER_SETUP_LOADED
#define ST7735_DRIVER
#define TFT_WIDTH 128
#define TFT_HEIGHT 160
#define ST7735_BLACKTAB

#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS 5
#define TFT_DC 21
#define TFT_RST 4

#define SPI_FREQUENCY 20000000

#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include "secrets.h"

WiFiClient wifiClient;
volatile bool wifiHasIp = false;

static const uint16_t SCREEN_W = 160;
static const uint16_t SCREEN_H = 128;
static const uint16_t RAW_W = 160;
static const uint16_t RAW_H = 128;
static const uint8_t RAW_X_OFFSET = 0;
static const uint8_t RAW_Y_OFFSET = 0;

// ESP32-CAM endpoint (default from CameraWebServer example)
String camHost = "192.168.4.1";
uint16_t camPort = 80;
String capturePath = "/capture";

uint32_t refreshMs = 1200;
uint32_t lastFrameMs = 0;
uint32_t frameFailCount = 0;
uint32_t frameOkCount = 0;
bool rawRenderInitialized = false;
static uint16_t frameBuffer[RAW_W * RAW_H];
uint8_t activeMadctl = 0xA0;
bool activeSwapBytes = true;
const bool blitLowByteFirst = true;

// Raw ST7735 commands for low-level display diagnostics (bypass TFT_eSPI).
static const uint8_t CMD_SWRESET = 0x01;
static const uint8_t CMD_SLPOUT = 0x11;
static const uint8_t CMD_COLMOD = 0x3A;
static const uint8_t CMD_MADCTL = 0x36;
static const uint8_t CMD_INVOFF = 0x20;
static const uint8_t CMD_NORON = 0x13;
static const uint8_t CMD_DISPON = 0x29;
static const uint8_t CMD_CASET = 0x2A;
static const uint8_t CMD_RASET = 0x2B;
static const uint8_t CMD_RAMWR = 0x2C;

const char *wifiStatusName(wl_status_t s) {
  switch (s) {
    case WL_NO_SHIELD: return "NO_SHIELD";
    case WL_IDLE_STATUS: return "IDLE";
    case WL_NO_SSID_AVAIL: return "NO_SSID";
    case WL_SCAN_COMPLETED: return "SCAN_DONE";
    case WL_CONNECTED: return "CONNECTED";
    case WL_CONNECT_FAILED: return "CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "CONNECTION_LOST";
    case WL_DISCONNECTED: return "DISCONNECTED";
    default: return "UNKNOWN";
  }
}

void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.printf("[WIFI-EVT] id=%d ", (int)event);
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_START:
      Serial.println("STA_START");
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("STA_CONNECTED");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      wifiHasIp = true;
      Serial.printf("GOT_IP %s\n", WiFi.localIP().toString().c_str());
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      wifiHasIp = false;
      Serial.printf("STA_DISCONNECTED reason=%d\n", info.wifi_sta_disconnected.reason);
      break;
    default:
      Serial.println("other");
      break;
  }
}

void diagStep(const char *step, const String &extra = "") {
  Serial.printf("[DIAG] %s", step);
  if (extra.length() > 0) {
    Serial.printf(" | %s", extra.c_str());
  }
  Serial.println();
}

void showDisplaySanityCheck() {
  Serial.println("[DIAG] Display sanity check shown");
}

void rawWriteCmd(uint8_t cmd) {
  digitalWrite(TFT_CS, LOW);
  digitalWrite(TFT_DC, LOW);
  SPI.write(cmd);
  digitalWrite(TFT_CS, HIGH);
}

void rawWriteData(const uint8_t *data, size_t len) {
  digitalWrite(TFT_CS, LOW);
  digitalWrite(TFT_DC, HIGH);
  SPI.writeBytes(data, len);
  digitalWrite(TFT_CS, HIGH);
}

void rawSetAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  x0 += RAW_X_OFFSET;
  x1 += RAW_X_OFFSET;
  y0 += RAW_Y_OFFSET;
  y1 += RAW_Y_OFFSET;

  uint8_t d[4];
  rawWriteCmd(CMD_CASET);
  d[0] = x0 >> 8;
  d[1] = x0 & 0xFF;
  d[2] = x1 >> 8;
  d[3] = x1 & 0xFF;
  rawWriteData(d, 4);

  rawWriteCmd(CMD_RASET);
  d[0] = y0 >> 8;
  d[1] = y0 & 0xFF;
  d[2] = y1 >> 8;
  d[3] = y1 & 0xFF;
  rawWriteData(d, 4);

  rawWriteCmd(CMD_RAMWR);
}

void rawFill565(uint16_t color) {
  rawSetAddrWindow(0, 0, RAW_W - 1, RAW_H - 1);

  // Write in chunks to avoid large allocations.
  static uint8_t line[RAW_W * 2];
  for (int i = 0; i < RAW_W; i++) {
    line[i * 2] = color >> 8;
    line[i * 2 + 1] = color & 0xFF;
  }

  digitalWrite(TFT_CS, LOW);
  digitalWrite(TFT_DC, HIGH);
  for (int y = 0; y < RAW_H; y++) {
    SPI.writeBytes(line, sizeof(line));
  }
  digitalWrite(TFT_CS, HIGH);
}

void rawSt7735InitCommon() {
  pinMode(TFT_CS, OUTPUT);
  pinMode(TFT_DC, OUTPUT);
  pinMode(TFT_RST, OUTPUT);

  digitalWrite(TFT_CS, HIGH);
  digitalWrite(TFT_DC, HIGH);
  digitalWrite(TFT_RST, HIGH);

  // Hardware reset.
  digitalWrite(TFT_RST, HIGH);
  delay(50);
  digitalWrite(TFT_RST, LOW);
  delay(120);
  digitalWrite(TFT_RST, HIGH);
  delay(120);

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));

  rawWriteCmd(CMD_SWRESET);
  delay(150);
  rawWriteCmd(CMD_SLPOUT);
  delay(150);

  // 16-bit color (RGB565)
  rawWriteCmd(CMD_COLMOD);
  const uint8_t colmod = 0x05;
  rawWriteData(&colmod, 1);
  delay(20);

  // Landscape orientation (matches SCREEN_W x SCREEN_H), RGB/BGR via activeMadctl.
  rawWriteCmd(CMD_MADCTL);
  rawWriteData(&activeMadctl, 1);

  rawWriteCmd(CMD_INVOFF);
  rawWriteCmd(CMD_NORON);
  delay(10);
  rawWriteCmd(CMD_DISPON);
  delay(120);
}

void applyColorMode(uint8_t mode) {
  switch (mode % 4) {
    case 0:
      activeMadctl = 0xA0;
      activeSwapBytes = true;
      break;
    case 1:
      activeMadctl = 0xA0;
      activeSwapBytes = false;
      break;
    case 2:
      activeMadctl = 0xA8;
      activeSwapBytes = true;
      break;
    default:
      activeMadctl = 0xA8;
      activeSwapBytes = false;
      break;
  }

  TJpgDec.setSwapBytes(activeSwapBytes);
  rawRenderInitialized = false;
  Serial.printf("[COLOR] madctl=0x%02X swap=%d\n", activeMadctl, activeSwapBytes ? 1 : 0);
}

void runRawDisplayDiagnostic() {
  Serial.println("[DIAG] RAW ST7735 diagnostic start (bypass TFT_eSPI)");
  rawSt7735InitCommon();

  rawFill565(0xF800);  // red
  delay(500);
  rawFill565(0x07E0);  // green
  delay(500);
  rawFill565(0x001F);  // blue
  delay(500);
  rawFill565(0x0000);  // black

  SPI.endTransaction();
  Serial.println("[DIAG] RAW ST7735 diagnostic end");
}

void showBootTestPattern() {
  Serial.println("[BOOT] LCD test pattern shown for 5s");
}

void showStatus(const char *line1, const String &line2 = "") {
  Serial.println(line1);
  if (line2.length() > 0) {
    Serial.println(line2);
  }
}

bool rawTftOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  if (w == 0 || h == 0) return true;
  if (x >= RAW_W || y >= RAW_H) return true;
  if (x < 0 || y < 0) return true;

  uint16_t drawW = w;
  uint16_t drawH = h;
  if (x + drawW > RAW_W) drawW = RAW_W - x;
  if (y + drawH > RAW_H) drawH = RAW_H - y;

  for (uint16_t row = 0; row < drawH; row++) {
    uint16_t *dst = &frameBuffer[(y + row) * RAW_W + x];
    uint16_t *src = bitmap + row * w;
    memcpy(dst, src, drawW * sizeof(uint16_t));
  }
  return true;
}

void rawBlitFrameBuffer() {
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
  rawSetAddrWindow(0, 0, RAW_W - 1, RAW_H - 1);

  static uint8_t line[RAW_W * 2];
  digitalWrite(TFT_CS, LOW);
  digitalWrite(TFT_DC, HIGH);
  for (uint16_t y = 0; y < RAW_H; y++) {
    for (uint16_t x = 0; x < RAW_W; x++) {
      uint16_t c = frameBuffer[y * RAW_W + x];
      if (blitLowByteFirst) {
        line[x * 2] = c & 0xFF;
        line[x * 2 + 1] = c >> 8;
      } else {
        line[x * 2] = c >> 8;
        line[x * 2 + 1] = c & 0xFF;
      }
    }
    SPI.writeBytes(line, sizeof(line));
  }
  digitalWrite(TFT_CS, HIGH);
  SPI.endTransaction();
}

void connectWiFi() {
  if (wifiHasIp) {
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  esp_wifi_disconnect();
  delay(50);

  Serial.printf("[WIFI] STA MAC: %s\n", WiFi.macAddress().c_str());
  wifi_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  strncpy(reinterpret_cast<char *>(cfg.sta.ssid), WIFI_SSID, sizeof(cfg.sta.ssid) - 1);
  strncpy(reinterpret_cast<char *>(cfg.sta.password), WIFI_PASS, sizeof(cfg.sta.password) - 1);
  cfg.sta.threshold.authmode = (strlen(WIFI_PASS) == 0) ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
  cfg.sta.pmf_cfg.capable = true;
  cfg.sta.pmf_cfg.required = false;

  esp_err_t setCfgErr = esp_wifi_set_config(WIFI_IF_STA, &cfg);
  Serial.printf("[WIFI] esp_wifi_set_config => %d\n", (int)setCfgErr);

  esp_err_t connErr = esp_wifi_connect();
  Serial.printf("[WIFI] esp_wifi_connect => %d\n", (int)connErr);

  showStatus("WiFi connecting...");
  Serial.printf("[WIFI] Connecting to SSID: %s\n", WIFI_SSID);

  uint32_t start = millis();
  uint32_t lastLog = 0;
  while (!wifiHasIp && millis() - start < 20000) {
    delay(250);

    if (millis() - lastLog >= 1000) {
      wl_status_t s = WiFi.status();
      Serial.printf("[WIFI] wait=%lus status=%d(%s)\n", (millis() - start) / 1000,
                    (int)s, wifiStatusName(s));
      lastLog = millis();
    }
  }

  if (wifiHasIp) {
    Serial.printf("[WIFI] Connected. Local IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("[WIFI] RSSI: %d dBm\n", WiFi.RSSI());
  } else {
    wl_status_t s = WiFi.status();
    Serial.printf("[WIFI] Connection failed. status=%d(%s)\n", (int)s, wifiStatusName(s));
  }
}

bool probeCamera() {
  HTTPClient http;
  String url = "http://" + camHost + ":" + String(camPort) + capturePath;
  http.setTimeout(5000);

  Serial.printf("[HTTP] Probing: %s\n", url.c_str());
  if (!http.begin(wifiClient, url)) {
    showStatus("HTTP begin failed");
    Serial.println("[HTTP] begin() failed");
    return false;
  }

  int code = http.GET();
  int len = http.getSize();
  http.end();

  if (code == HTTP_CODE_OK && len > 0) {
    showStatus("Camera HTTP OK", String("Len: ") + len);
    Serial.printf("[HTTP] OK. code=%d len=%d\n", code, len);
    delay(1500);
    return true;
  }

  showStatus("Camera HTTP FAIL", String("Code: ") + code);
  Serial.printf("[HTTP] FAIL. code=%d len=%d\n", code, len);
  delay(1500);
  return false;
}

uint8_t chooseScale(uint16_t jpgW, uint16_t jpgH, uint16_t targetW, uint16_t targetH) {
  // TJpg_Decoder scale: 0=1/1, 1=1/2, 2=1/4, 3=1/8
  for (uint8_t scale = 0; scale <= 3; scale++) {
    uint16_t w = jpgW >> scale;
    uint16_t h = jpgH >> scale;
    if (w <= targetW && h <= targetH) {
      return scale;
    }
  }
  return 3;
}

bool fetchAndDrawFrame() {
  const uint8_t *jpgBuf = nullptr;
  int offset = 0;
  bool freeWhenDone = false;

  HTTPClient http;
  String url = "http://" + camHost + ":" + String(camPort) + capturePath;

  http.setTimeout(5000);
  if (!http.begin(wifiClient, url)) {
    return false;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[FRAME] HTTP error: %d\n", httpCode);
    http.end();
    return false;
  }

  int len = http.getSize();
  const int fallbackCap = 80 * 1024;
  int allocSize = (len > 0) ? len : fallbackCap;

  uint8_t *downloadBuf = (uint8_t *)malloc(allocSize);
  if (!downloadBuf) {
    Serial.printf("[FRAME] Out of memory (alloc=%d)\n", allocSize);
    http.end();
    return false;
  }

  WiFiClient *stream = http.getStreamPtr();
  uint32_t start = millis();
  while (http.connected() && offset < allocSize && millis() - start < 7000) {
    int available = stream->available();
    if (available > 0) {
      int toRead = min(available, allocSize - offset);
      int readCount = stream->readBytes(downloadBuf + offset, toRead);
      if (readCount > 0) offset += readCount;
    }
    delay(1);
  }
  http.end();

  if (offset <= 0) {
    Serial.println("[FRAME] No JPEG bytes read");
    free(downloadBuf);
    showStatus("Image read failed");
    return false;
  }
  if (len > 0 && offset < len) {
    Serial.printf("[FRAME] Partial JPEG read: %d/%d bytes\n", offset, len);
  }

  jpgBuf = downloadBuf;
  freeWhenDone = true;

  uint16_t jpgW = 0, jpgH = 0;
  TJpgDec.getJpgSize(&jpgW, &jpgH, jpgBuf, offset);
  if (jpgW == 0 || jpgH == 0) {
    Serial.printf("[FRAME] Invalid JPEG size from %d bytes\n", offset);
    if (freeWhenDone) {
      free((void *)jpgBuf);
    }
    return false;
  }
  uint8_t scale = chooseScale(jpgW, jpgH, SCREEN_W, SCREEN_H);
  uint16_t drawW = jpgW >> scale;
  uint16_t drawH = jpgH >> scale;
  while ((drawW > SCREEN_W || drawH > SCREEN_H) && scale < 3) {
    scale++;
    drawW = jpgW >> scale;
    drawH = jpgH >> scale;
  }

  Serial.printf("[FRAME] jpeg=%ux%u scale=%u draw=%ux%u bytes=%d\n",
                jpgW, jpgH, scale, drawW, drawH, offset);

  TJpgDec.setJpgScale(1 << scale);  // setJpgScale expects 1/2/4/8, not 0/1/2/3

  int16_t x = (drawW >= SCREEN_W) ? 0 : (SCREEN_W - drawW) / 2;
  int16_t y = (drawH >= SCREEN_H) ? 0 : (SCREEN_H - drawH) / 2;

  memset(frameBuffer, 0, sizeof(frameBuffer));
  int drawRc = TJpgDec.drawJpg(x, y, jpgBuf, offset);

  if (freeWhenDone) {
    free((void *)jpgBuf);
  }
  if (drawRc == 0) {
    Serial.println("[FRAME] JPEG draw rc=0 (continuing)");
  }

  if (!rawRenderInitialized) {
    rawSt7735InitCommon();
    SPI.endTransaction();
    rawRenderInitialized = true;
  }
  rawBlitFrameBuffer();

  frameOkCount++;

  return true;
}

void setup() {
  Serial.begin(115200);
  delay(100);
  WiFi.onEvent(onWiFiEvent);

  Serial.println();
  Serial.println("[BOOT] Receiver starting");
  Serial.printf("[BOOT] Build: %s %s\n", __DATE__, __TIME__);
  Serial.printf("[BOOT] Free heap at start: %u\n", ESP.getFreeHeap());

  // First, prove wiring/controller communication without TFT_eSPI config.
  runRawDisplayDiagnostic();

  TJpgDec.setCallback(rawTftOutput);
  applyColorMode(0);

  diagStep("Display init complete");
  delay(400);
  showDisplaySanityCheck();

  diagStep("Boot pattern test");
  showBootTestPattern();

  diagStep("WiFi phase", WIFI_SSID);
  connectWiFi();
  if (wifiHasIp) {
    diagStep("Camera probe", camHost + ":" + String(camPort));
    probeCamera();
  } else {
    diagStep("WiFi not connected", "Will retry in loop()");
  }
}

void loop() {
  if (!wifiHasIp) {
    connectWiFi();
    delay(500);
    return;
  }

  uint32_t now = millis();
  if (now - lastFrameMs >= refreshMs) {
    lastFrameMs = now;
    if (!fetchAndDrawFrame()) {
      frameFailCount++;
      Serial.printf("[FRAME] Fetch failed count=%lu\n", frameFailCount);
    }
  }
}
