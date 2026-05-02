#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_err.h>

const char *ssid = "ESP32CAM-LINK";
char pass[] = "camdisplay123";
const bool useIdfConnect = true;

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
  Serial.printf("[EVT] id=%d ", (int)event);
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_START:
      Serial.println("STA_START");
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("STA_CONNECTED");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.printf("GOT_IP %s\n", WiFi.localIP().toString().c_str());
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.printf("STA_DISCONNECTED reason=%d\n", info.wifi_sta_disconnected.reason);
      break;
    default:
      Serial.println("other");
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("[TEST] wifi_sta_test starting");
  Serial.printf("[TEST] Target SSID: %s\n", ssid);

  WiFi.onEvent(onWiFiEvent);
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setSleep(false);

  if (useIdfConnect) {
    Serial.println("[TEST] connect path: ESP-IDF config/connect");
    wifi_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));

    strncpy(reinterpret_cast<char *>(cfg.sta.ssid), ssid, sizeof(cfg.sta.ssid) - 1);
    strncpy(reinterpret_cast<char *>(cfg.sta.password), pass, sizeof(cfg.sta.password) - 1);

    cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    cfg.sta.pmf_cfg.capable = true;
    cfg.sta.pmf_cfg.required = false;

    esp_err_t setCfgErr = esp_wifi_set_config(WIFI_IF_STA, &cfg);
    Serial.printf("[TEST] esp_wifi_set_config => %d\n", (int)setCfgErr);

    esp_err_t connErr = esp_wifi_connect();
    Serial.printf("[TEST] esp_wifi_connect => %d\n", (int)connErr);
  } else {
    Serial.println("[TEST] connect path: WiFi.begin");
    if (strlen(pass) == 0) WiFi.begin(ssid);
    else WiFi.begin(ssid, pass);
  }

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    wl_status_t s = WiFi.status();
    Serial.printf("[TEST] wait=%lus status=%d(%s)\n", (millis() - start) / 1000, (int)s, wifiStatusName(s));
    delay(1000);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[TEST] CONNECTED IP=%s RSSI=%d\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
  } else {
    Serial.printf("[TEST] FAILED status=%d(%s)\n", (int)WiFi.status(), wifiStatusName(WiFi.status()));
  }
}

void loop() {
  delay(2000);
}
