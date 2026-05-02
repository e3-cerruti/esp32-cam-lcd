# ESP32 + ST7735 (MicroPython)

This project runs a simple RGB color-cycle test on an ST7735 display using an ESP32.

## Project structure

- `main.py` → App entrypoint (sets up SPI + pins and runs the test)
- `lib/st7735.py` → Lightweight ST7735 driver

## Wiring (from `main.py`)

- `SCK`  -> GPIO 18
- `MOSI` -> GPIO 23
- `CS`   -> GPIO 5
- `DC`   -> GPIO 2
- `RST`  -> GPIO 4
- `MISO` is not required for this display test.

## Run on board

1. Flash MicroPython firmware for ESP32 (if not already flashed).
2. Copy project files to the board.

Example with `mpremote`:

```bash
mpremote connect auto fs cp main.py :main.py
mpremote connect auto fs mkdir :lib
mpremote connect auto fs cp lib/st7735.py :lib/st7735.py
mpremote connect auto reset
```

After reset, the display should cycle through red, green, and blue, then clear to black.
