#ifndef CAMERA_INDEX_H
#define CAMERA_INDEX_H

#include <pgmspace.h>

static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>ESP32-CAM</title>
  <style>
    body { background:#111; color:#eee; font-family:Arial,sans-serif; margin:0; padding:16px; }
    .wrap { max-width:900px; margin:auto; }
    img { width:100%; height:auto; border:1px solid #333; border-radius:8px; }
    .row { display:flex; gap:12px; margin-bottom:12px; }
    a, button { background:#2c7; color:#012; border:none; border-radius:6px; padding:10px 12px; text-decoration:none; font-weight:600; cursor:pointer; }
  </style>
</head>
<body>
  <div class="wrap">
    <h2>ESP32-CAM Stream</h2>
    <div class="row">
      <a href="/capture" target="_blank">Capture JPEG</a>
      <button onclick="toggleStream()">Toggle Stream</button>
    </div>
    <img id="stream" src="/stream" alt="stream" />
  </div>
  <script>
    let on = true;
    function toggleStream() {
      const img = document.getElementById('stream');
      on = !on;
      img.src = on ? '/stream?t=' + Date.now() : '';
    }
  </script>
</body>
</html>
)rawliteral";

#endif  // CAMERA_INDEX_H
