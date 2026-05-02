# ESP32-CAM settings for LCD-friendly images

Use the standard `CameraWebServer` Arduino example on the ESP32-CAM, with these settings:

1. Keep JPEG output enabled:
   - `config.pixel_format = PIXFORMAT_JPEG;`

2. Force a small frame for quick transfer/display:
   - `s->set_framesize(s, FRAMESIZE_QVGA);`  (320x240)

3. Keep reasonable JPEG quality:
   - `s->set_quality(s, 12);`  (lower number = better quality, larger files)

4. Make sure these endpoints exist and are reachable from another ESP32:
   - `http://<cam-ip>/capture`  (single JPEG frame)
   - optional: `http://<cam-ip>/stream` (MJPEG stream)

Recommended first test:
- Open `http://<cam-ip>/capture` in a browser on your laptop and confirm you receive an image quickly.

If frames are too slow on LCD receiver:
- use `FRAMESIZE_QQVGA` (160x120), and/or
- increase quality value (e.g. `14` or `16`) to reduce file size.
