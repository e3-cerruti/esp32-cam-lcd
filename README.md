# Team Runners Prototype Support Repo

This workspace contains the ESP32 prototype work used to support the **Team Runners** student invention concept.

The main build in this repository uses an `ESP32-CAM` as a wireless image source and a second `ESP32` plus `ST7735` TFT as a compact receiver/display demo. The goal is to help students explain, demonstrate, and iterate on a wearable safety concept for runners.

## Start Here

For most people, the main project is in [arduino-cam-lcd](arduino-cam-lcd/README.md).

Key documents:

- [Student build guide](arduino-cam-lcd/docs/student-guide.md)
- [ESP32-CAM setup guide for Mac](CameraWebServer/docs/ESP32-CAM%20Setup%20Guide%20for%20Mac.md)
- [Project-specific camera modifications](CameraWebServer/docs/ESP32-CAM%20Project%20Modifications.md)
- [Camera settings notes](arduino-cam-lcd/docs/esp32-cam-settings.md)

## Repository Layout

### Main Arduino prototype

- [arduino-cam-lcd/README.md](arduino-cam-lcd/README.md) — main project overview
- [arduino-cam-lcd/receiver/receiver.ino](arduino-cam-lcd/receiver/receiver.ino) — ESP32 receiver firmware for the ST7735 display
- [CameraWebServer/CameraWebServer.ino](CameraWebServer/CameraWebServer.ino) — modified ESP32-CAM firmware
- [arduino-cam-lcd/docs/student-guide.md](arduino-cam-lcd/docs/student-guide.md) — student-facing wiring and build document

### Camera documentation

- [CameraWebServer/docs/ESP32-CAM Setup Guide for Mac.md](CameraWebServer/docs/ESP32-CAM%20Setup%20Guide%20for%20Mac.md) — stock bring-up workflow for the camera board
- [CameraWebServer/docs/ESP32-CAM Project Modifications.md](CameraWebServer/docs/ESP32-CAM%20Project%20Modifications.md) — how the camera example was adapted for this project

### Earlier display experiments

- [main.py](main.py)
- [st7735.py](st7735.py)
- [lib/st7735.py](lib/st7735.py)

These MicroPython files are older display experiments kept for reference. They are not the primary path for the current Team Runners demo.

## Prototype Summary

The current Arduino prototype works like this:

1. The camera board creates a Wi-Fi access point.
2. The receiver board joins that access point.
3. The receiver repeatedly fetches JPEG frames from the camera.
4. The receiver decodes and draws the image on the ST7735 TFT.

This provides a compact proof-of-concept visual pipeline that students can use in presentations and demonstrations.
