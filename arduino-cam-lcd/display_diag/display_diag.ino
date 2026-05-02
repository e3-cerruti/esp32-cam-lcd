#include <Arduino.h>
#include <SPI.h>

#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS 5
#define TFT_DC 21
#define TFT_RST 4

#define SPI_FREQUENCY 20000000

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

struct StageConfig {
  const char *name;
  uint8_t madctl;
  uint16_t width;
  uint16_t height;
  uint8_t xOffset;
  uint8_t yOffset;
};

StageConfig stages[] = {
  {"S1 native C0 off0,0", 0xC0, 128, 160, 0, 0},
  {"S2 native C0 off2,1", 0xC0, 128, 160, 2, 1},
  {"S3 native C8 off0,0", 0xC8, 128, 160, 0, 0},
  {"S4 landscape A0 off0,0", 0xA0, 160, 128, 0, 0},
  {"S5 landscape A0 off1,2", 0xA0, 160, 128, 1, 2},
  {"S6 landscape A8 off0,0", 0xA8, 160, 128, 0, 0},
};

void writeCmd(uint8_t cmd) {
  digitalWrite(TFT_CS, LOW);
  digitalWrite(TFT_DC, LOW);
  SPI.write(cmd);
  digitalWrite(TFT_CS, HIGH);
}

void writeData(const uint8_t *data, size_t len) {
  digitalWrite(TFT_CS, LOW);
  digitalWrite(TFT_DC, HIGH);
  SPI.writeBytes(data, len);
  digitalWrite(TFT_CS, HIGH);
}

void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t xOffset, uint8_t yOffset) {
  x0 += xOffset;
  x1 += xOffset;
  y0 += yOffset;
  y1 += yOffset;

  uint8_t d[4];
  writeCmd(CMD_CASET);
  d[0] = x0 >> 8;
  d[1] = x0 & 0xFF;
  d[2] = x1 >> 8;
  d[3] = x1 & 0xFF;
  writeData(d, 4);

  writeCmd(CMD_RASET);
  d[0] = y0 >> 8;
  d[1] = y0 & 0xFF;
  d[2] = y1 >> 8;
  d[3] = y1 & 0xFF;
  writeData(d, 4);

  writeCmd(CMD_RAMWR);
}

void fillRectRaw(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color, const StageConfig &cfg) {
  if (w == 0 || h == 0) return;
  if (x >= cfg.width || y >= cfg.height) return;
  if (x + w > cfg.width) w = cfg.width - x;
  if (y + h > cfg.height) h = cfg.height - y;

  setAddrWindow(x, y, x + w - 1, y + h - 1, cfg.xOffset, cfg.yOffset);

  uint8_t hi = color >> 8;
  uint8_t lo = color & 0xFF;
  static uint8_t line[160 * 2];
  for (uint16_t i = 0; i < w; i++) {
    line[i * 2] = hi;
    line[i * 2 + 1] = lo;
  }

  digitalWrite(TFT_CS, LOW);
  digitalWrite(TFT_DC, HIGH);
  for (uint16_t row = 0; row < h; row++) {
    SPI.writeBytes(line, w * 2);
  }
  digitalWrite(TFT_CS, HIGH);
}

void fillScreenRaw(uint16_t color, const StageConfig &cfg) {
  fillRectRaw(0, 0, cfg.width, cfg.height, color, cfg);
}

void initPanel(const StageConfig &cfg) {
  pinMode(TFT_CS, OUTPUT);
  pinMode(TFT_DC, OUTPUT);
  pinMode(TFT_RST, OUTPUT);

  digitalWrite(TFT_CS, HIGH);
  digitalWrite(TFT_DC, HIGH);
  digitalWrite(TFT_RST, HIGH);

  digitalWrite(TFT_RST, HIGH);
  delay(50);
  digitalWrite(TFT_RST, LOW);
  delay(120);
  digitalWrite(TFT_RST, HIGH);
  delay(120);

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));

  writeCmd(CMD_SWRESET);
  delay(150);
  writeCmd(CMD_SLPOUT);
  delay(150);

  writeCmd(CMD_COLMOD);
  const uint8_t colmod = 0x05;
  writeData(&colmod, 1);
  delay(20);

  writeCmd(CMD_MADCTL);
  writeData(&cfg.madctl, 1);

  writeCmd(CMD_INVOFF);
  writeCmd(CMD_NORON);
  delay(10);
  writeCmd(CMD_DISPON);
  delay(120);
}

void drawStage(const StageConfig &cfg, uint8_t index) {
  Serial.println();
  Serial.printf("=== %s ===\n", cfg.name);
  Serial.printf("madctl=0x%02X size=%ux%u offset=%u,%u\n",
                cfg.madctl, cfg.width, cfg.height, cfg.xOffset, cfg.yOffset);

  initPanel(cfg);

  fillScreenRaw(0x0000, cfg);

  fillRectRaw(0, 0, cfg.width, 12, 0xF800, cfg);
  fillRectRaw(0, cfg.height - 12, cfg.width, 12, 0x07E0, cfg);
  fillRectRaw(0, 0, 12, cfg.height, 0x001F, cfg);
  fillRectRaw(cfg.width - 12, 0, 12, cfg.height, 0xFFE0, cfg);

  fillRectRaw(cfg.width / 2 - 20, cfg.height / 2 - 20, 40, 40, 0xFFFF, cfg);
  fillRectRaw(cfg.width / 2 - 30, cfg.height / 2 - 2, 60, 4, 0x07FF, cfg);
  fillRectRaw(cfg.width / 2 - 2, cfg.height / 2 - 30, 4, 60, 0xF81F, cfg);

  fillRectRaw(16, 16, 16, 16, 0xFFFF, cfg);
  fillRectRaw(cfg.width - 32, 16, 16, 16, 0xFFFF, cfg);
  fillRectRaw(16, cfg.height - 32, 16, 16, 0xFFFF, cfg);
  fillRectRaw(cfg.width - 32, cfg.height - 32, 16, 16, 0xFFFF, cfg);

  // Big stage marker bars (1..6) so user can identify stage without serial.
  uint16_t markerY = (cfg.height > 42) ? (cfg.height - 42) : 0;
  for (uint8_t i = 0; i <= index; i++) {
    fillRectRaw(8 + i * 18, markerY, 12, 30, 0xFFFF, cfg);
  }

  SPI.endTransaction();
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("display_diag starting");
}

void loop() {
  static size_t stageIndex = 0;
  drawStage(stages[stageIndex], (uint8_t)stageIndex);
  stageIndex = (stageIndex + 1) % (sizeof(stages) / sizeof(stages[0]));
  delay(6000);
}
