#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Fetch a JPEG from the ESP32-CAM and inspect it.")
    parser.add_argument("--url", default="http://192.168.4.1/capture", help="Camera capture URL")
    parser.add_argument(
        "--out-dir",
        default=str(Path(__file__).resolve().parents[1] / "camera_fetch_output"),
        help="Output directory for fetched files",
    )
    parser.add_argument("--timeout", type=float, default=10.0, help="HTTP timeout in seconds")
    return parser.parse_args()


def read_be16(data: bytes, offset: int) -> int:
    return (data[offset] << 8) | data[offset + 1]


def parse_jpeg_segments(data: bytes) -> dict:
    info: dict = {
        "length": len(data),
        "markers": [],
        "sof": None,
        "app0": None,
        "app1": None,
        "comment": None,
    }

    if len(data) < 4 or data[0:2] != b"\xFF\xD8":
        raise ValueError("Not a JPEG: missing SOI marker")

    i = 2
    while i < len(data):
        if data[i] != 0xFF:
            i += 1
            continue

        while i < len(data) and data[i] == 0xFF:
            i += 1
        if i >= len(data):
            break

        marker = data[i]
        i += 1

        # Standalone markers.
        if marker in {0x01} or 0xD0 <= marker <= 0xD9:
            info["markers"].append({"marker": f"FF{marker:02X}", "standalone": True})
            if marker == 0xD9:
                break
            continue

        if i + 2 > len(data):
            break
        seg_len = read_be16(data, i)
        if seg_len < 2 or i + seg_len > len(data):
            break
        payload = data[i + 2 : i + seg_len]
        name = f"FF{marker:02X}"
        entry = {"marker": name, "length": seg_len}
        info["markers"].append(entry)

        if marker == 0xE0 and payload[:5] == b"JFIF\x00":
            info["app0"] = {
                "identifier": "JFIF",
                "version": f"{payload[5]}.{payload[6]}",
                "density_units": payload[7],
                "x_density": read_be16(payload, 8),
                "y_density": read_be16(payload, 10),
            }
        elif marker == 0xE1:
            try:
                text = payload[:64].decode("latin1", errors="replace")
            except Exception:
                text = ""
            info["app1"] = {"preview": text}
        elif marker == 0xFE:
            info["comment"] = payload.decode("latin1", errors="replace")
        elif marker in {0xC0, 0xC1, 0xC2, 0xC3, 0xC5, 0xC6, 0xC7, 0xC9, 0xCA, 0xCB, 0xCD, 0xCE, 0xCF}:
            components = []
            if len(payload) >= 6:
                component_count = payload[5]
                p = 6
                for _ in range(component_count):
                    if p + 3 <= len(payload):
                        cid = payload[p]
                        samp = payload[p + 1]
                        qt = payload[p + 2]
                        components.append(
                            {
                                "id": cid,
                                "h_samp": samp >> 4,
                                "v_samp": samp & 0x0F,
                                "qt": qt,
                            }
                        )
                    p += 3
            info["sof"] = {
                "marker": name,
                "precision": payload[0],
                "height": read_be16(payload, 1),
                "width": read_be16(payload, 3),
                "components": components,
            }
        elif marker == 0xDA:
            # Start of scan; compressed data follows until EOI.
            break

        i += seg_len

    return info


def try_decode_with_pillow(jpeg_path: Path, out_dir: Path) -> str | None:
    try:
        from PIL import Image
    except Exception:
        return None

    png_path = out_dir / f"{jpeg_path.stem}_decoded.png"
    with Image.open(jpeg_path) as img:
        img.save(png_path)
    return str(png_path)


def main() -> int:
    args = parse_args()
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    ts = time.strftime("%Y%m%d_%H%M%S")
    jpg_path = out_dir / f"capture_{ts}.jpg"
    json_path = out_dir / f"capture_{ts}_metadata.json"

    print(f"Fetching: {args.url}")
    try:
        with urllib.request.urlopen(args.url, timeout=args.timeout) as resp:
            data = resp.read()
            headers = dict(resp.getheaders())
            status = getattr(resp, "status", None)
    except urllib.error.URLError as exc:
        print(f"Fetch failed: {exc}", file=sys.stderr)
        return 2

    jpg_path.write_bytes(data)
    metadata = parse_jpeg_segments(data)
    metadata["url"] = args.url
    metadata["http_status"] = status
    metadata["http_headers"] = headers
    metadata["saved_jpeg"] = str(jpg_path)

    png_path = try_decode_with_pillow(jpg_path, out_dir)
    if png_path:
        metadata["decoded_png"] = png_path

    json_path.write_text(json.dumps(metadata, indent=2), encoding="utf-8")

    print(f"Saved JPEG: {jpg_path}")
    print(f"Saved metadata: {json_path}")
    if png_path:
        print(f"Saved decoded PNG: {png_path}")
    else:
        print("Decoded PNG not created (Pillow not available).")

    sof = metadata.get("sof") or {}
    if sof:
        print(f"JPEG size: {sof.get('width')}x{sof.get('height')}")
        print(f"SOF marker: {sof.get('marker')}")
        print(f"Components: {sof.get('components')}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
