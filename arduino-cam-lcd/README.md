# ESP32-CAM -> ESP32 LCD Viewer (Arduino)

This project pulls JPEG frames from an `ESP32-CAM` running `CameraWebServer` and displays them on an SPI LCD.

## Folder layout

- `receiver/receiver.ino` : ESP32 + LCD sketch (fetches `/capture` and draws JPEG)
- `receiver/secrets.h` : Wi-Fi credentials
- `docs/esp32-cam-settings.md` : required ESP32-CAM camera settings

## Libraries needed in Arduino IDE

Install:
- `TFT_eSPI` (Bodmer)
- `TJpg_Decoder` (Bodmer)

Also ensure ESP32 board package is installed.

## LCD setup

`receiver.ino` uses `TFT_eSPI`. Configure your panel pins/driver in `User_Setup.h` or a setup file.

For ST7735 (typical), ensure:
- driver and pins match your wiring
- display size matches sketch constants (`160x128`)

## Receiver configuration

In `receiver/receiver.ino`, set:
- `camHost` to your ESP32-CAM IP address
- `camPort` (usually `80`)
- `capturePath` (`/capture`)

Wi-Fi is already set in `receiver/secrets.h`:
- SSID: `e3CivicHigh`
- Password: `e3Griffins!`

## Build/run flow

1. Flash the ESP32-CAM with CameraWebServer and settings from [docs/esp32-cam-settings.md](docs/esp32-cam-settings.md).
2. Verify `http://<cam-ip>/capture` works in browser.
3. Flash `receiver/receiver.ino` to the ESP32 connected to your LCD.
4. Set `camHost` to the camera IP and reboot receiver.

## Notes

- Current implementation fetches still images repeatedly (not MJPEG decode).
- Works well for status/preview views.
- For higher FPS, use smaller frame size (QQVGA) and shorter refresh interval.
