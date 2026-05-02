# Fritzing Build Checklist

Use this checklist to build a clean breadboard diagram in Fritzing.

## 1. Place parts
- ESP32 dev board (receiver)
- ST7735 TFT display
- Breadboard (optional visual anchor)

## 2. Assign wire colors
- Power rails: red (3V3), black (GND)
- SPI/control:
  - CS = yellow
  - RESET = orange
  - A0/DC = green
  - SDA/MOSI = blue
  - SCK = white

## 3. Critical pin mapping
- A0/DC must go to GPIO21 (D21)
- Do not use GPIO2 for A0/DC in this project

## 4. Add labels in the drawing
- Label each TFT pin and matching ESP32 pin
- Add note: "Camera path is wireless; no direct wires from camera to receiver TFT"

## 5. Export for students
- Export PNG for handout
- Keep source file for edits
- Include [wiring-table.csv](wiring-table.csv) with the worksheet
