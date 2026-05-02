#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "camera_fetch_output"
HEADER_PATH = ROOT / "receiver" / "test_card_jpg.h"
JPG_PATH = OUT_DIR / "test_card_320x240_v2.jpg"

W, H = 320, 240

img = Image.new("RGB", (W, H), "black")
d = ImageDraw.Draw(img)

# Top: color bars
colors = [
    (255, 0, 0),
    (0, 255, 0),
    (0, 0, 255),
    (0, 255, 255),
    (255, 0, 255),
    (255, 255, 0),
    (255, 255, 255),
    (0, 0, 0),
]
bar_w = W // len(colors)
for i, color in enumerate(colors):
    x0 = i * bar_w
    x1 = W if i == len(colors) - 1 else (i + 1) * bar_w
    d.rectangle([x0, 0, x1 - 1, 79], fill=color)

# Middle: grayscale ramp
for x in range(W):
    v = int(round(255 * x / (W - 1)))
    d.line([(x, 80), (x, 159)], fill=(v, v, v))

# Bottom left: skin-ish / warm / cool patches
patches = [
    ((214, 170, 140), "skin"),
    ((255, 128, 64), "warm"),
    ((64, 160, 255), "cool"),
    ((32, 32, 32), "dark"),
]
patch_w = W // 4
for i, (color, _name) in enumerate(patches):
    x0 = i * patch_w
    x1 = W if i == 3 else (i + 1) * patch_w
    d.rectangle([x0, 160, x1 - 1, H - 1], fill=color)

# Orientation markers / edges
for y in range(H):
    img.putpixel((0, y), (255, 255, 255))
    img.putpixel((W - 1, y), (255, 255, 255))
for x in range(W):
    img.putpixel((x, 0), (255, 255, 255))
    img.putpixel((x, H - 1), (255, 255, 255))

# Center crosshair
cx, cy = W // 2, H // 2
for dx in range(-12, 13):
    img.putpixel((cx + dx, cy), (255, 255, 255))
for dy in range(-12, 13):
    img.putpixel((cx, cy + dy), (255, 255, 255))

OUT_DIR.mkdir(parents=True, exist_ok=True)
img.save(JPG_PATH, format="JPEG", quality=90, subsampling="4:2:2")

data = JPG_PATH.read_bytes()
HEADER_PATH.write_text(
    "#pragma once\n\n"
    f"const unsigned int test_card_jpg_len = {len(data)};\n"
    "const unsigned char test_card_jpg[] = {\n    "
    + ", ".join(f"0x{b:02X}" for b in data)
    + "\n};\n",
    encoding="utf-8",
)

print(f"Wrote {JPG_PATH}")
print(f"Wrote {HEADER_PATH}")
print(f"JPEG bytes: {len(data)}")
