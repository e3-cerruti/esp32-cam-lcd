# ESP32-CAM Project Modifications

This document explains how the standard Arduino `CameraWebServer` example was modified for the ESP32-CAM to ST7735 receiver project.

## Purpose

The original setup guide gets the camera board assembled, programmed, and streaming in a browser.

For this project, the camera is not used as a normal classroom Wi-Fi client. Instead, it is configured as a dedicated image source for the ESP32 receiver and ST7735 display.

## Summary of Changes

### 1. Dedicated SoftAP mode

Instead of joining an existing Wi-Fi network, the camera creates its own wireless network.

- SSID: `ESP32CAM-LINK`
- Password: `camdisplay123`
- Channel: `6`
- Max clients: `1`

This allows the receiver board to connect directly to the camera without depending on school or home Wi-Fi.

## 2. AI Thinker board selection

The project is configured for the AI Thinker ESP32-CAM board in `board_config.h`.

Active define:
- `CAMERA_MODEL_AI_THINKER`

## 3. Lower frame size for the LCD pipeline

The camera output is reduced to match the receiver/display workflow more closely and reduce bandwidth.

- Frame size: `QQVGA`
- Actual image size: `160x120`

This is a better fit for the ST7735 display path than the larger default example resolutions.

## 4. JPEG tuned for transmission

The JPEG quality is set to a lower-bandwidth value for more reliable transfer to the receiver.

- JPEG quality: `16`

## 5. Sensor color tuning

Several sensor settings were adjusted to improve how the image looks on the TFT display.

Applied settings:
- Brightness: `1`
- Contrast: `1`
- Saturation: `2`
- AE level: `1`
- AEC2: enabled
- AWB gain: enabled
- White balance: enabled

## 6. Receiver-driven display workflow

The camera still hosts the normal web server, but in this project the main consumer is the receiver firmware.

The receiver:
- joins the camera SoftAP
- requests JPEG images from `/capture`
- decodes them
- draws them to the ST7735 screen over SPI

## 7. Why these changes were made

These modifications were made to improve:

- reliability
- startup simplicity
- bandwidth usage
- refresh speed
- compatibility with the 160x128 ST7735 display

## File Locations

Main camera firmware:
- [CameraWebServer/CameraWebServer.ino](../CameraWebServer.ino)

Board selection:
- [CameraWebServer/board_config.h](../board_config.h)

Receiver firmware using the camera stream:
- [arduino-cam-lcd/receiver/receiver.ino](../../arduino-cam-lcd/receiver/receiver.ino)

## Suggested Use

- Use [CameraWebServer/docs/ESP32-CAM Setup Guide for Mac.md](ESP32-CAM%20Setup%20Guide%20for%20Mac.md) for first-time board bring-up.
- Use this document to explain how the stock camera example was adapted for the final project.
