# ESP32-CAM to ST7735 Receiver Prototype

This project is the main Arduino prototype used to support the **Team Runners** student invention concept.

An `ESP32-CAM` captures images and serves them over Wi-Fi. A second `ESP32` downloads those JPEG frames and draws them to a `160x128` `ST7735` SPI display.

## Documentation

- [Student build guide](docs/student-guide.md)
- [Camera settings notes](docs/esp32-cam-settings.md)
- [ESP32-CAM setup guide for Mac](../CameraWebServer/docs/ESP32-CAM%20Setup%20Guide%20for%20Mac.md)
- [ESP32-CAM project modifications](../CameraWebServer/docs/ESP32-CAM%20Project%20Modifications.md)

## Main Files

- [receiver/receiver.ino](receiver/receiver.ino) — receiver firmware for the ESP32 + ST7735 display
- [receiver/secrets.h](receiver/secrets.h) — receiver Wi-Fi credentials for the camera SoftAP
- [docs/student-guide.md](docs/student-guide.md) — student-facing wiring, image, and build notes
- [docs/esp32-cam-settings.md](docs/esp32-cam-settings.md) — camera-side notes

## Hardware Roles

### Camera board

- Board: `ESP32-CAM` (AI Thinker)
- Firmware: [../CameraWebServer/CameraWebServer.ino](../CameraWebServer/CameraWebServer.ino)
- Role: create a dedicated Wi-Fi network and serve JPEG images from `/capture`

### Receiver board

- Board: standard `ESP32` dev board
- Firmware: [receiver/receiver.ino](receiver/receiver.ino)
- Role: connect to the camera, fetch frames, decode JPEG, and display them on the TFT

## Default Network Configuration

The current project uses a direct camera-to-receiver wireless link.

Camera SoftAP:

- SSID: `ESP32CAM-LINK`
- Password: `camdisplay123`
- Camera IP: `192.168.4.1`

Receiver defaults:

- capture endpoint: `/capture`
- port: `80`

## Display Wiring

The verified student wiring is documented in [docs/student-guide.md](docs/student-guide.md).

Receiver pin assignments in the current firmware:

- `TFT_MOSI` → `23`
- `TFT_SCLK` → `18`
- `TFT_CS` → `5`
- `TFT_DC` → `21`
- `TFT_RST` → `4`

## Arduino Libraries Needed

Install in Arduino IDE:

- `TFT_eSPI` by Bodmer
- `TJpg_Decoder` by Bodmer

Also install the `ESP32` board package from Espressif.

## Bring-Up Flow

1. Set up and verify the camera board using [../CameraWebServer/docs/ESP32-CAM Setup Guide for Mac.md](../CameraWebServer/docs/ESP32-CAM%20Setup%20Guide%20for%20Mac.md).
2. Review the project-specific camera changes in [../CameraWebServer/docs/ESP32-CAM Project Modifications.md](../CameraWebServer/docs/ESP32-CAM%20Project%20Modifications.md).
3. Flash [../CameraWebServer/CameraWebServer.ino](../CameraWebServer/CameraWebServer.ino) to the ESP32-CAM.
4. Flash [receiver/receiver.ino](receiver/receiver.ino) to the ESP32 receiver.
5. Wire the display using [docs/student-guide.md](docs/student-guide.md).
6. Power both boards and confirm the receiver displays the camera image.

## Notes

- The receiver repeatedly fetches still JPEG frames; this is not an MJPEG video decoder.
- The camera firmware is tuned for `QQVGA` (`160x120`) to reduce bandwidth and better match the TFT display pipeline.
- This repository also contains earlier MicroPython display experiments at the workspace root, but this folder is the main implementation for the current prototype.
