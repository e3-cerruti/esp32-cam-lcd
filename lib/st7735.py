"""Minimal ST7735 driver for MicroPython.

This driver supports:
- 128x160 ST7735 displays
- RGB565 color mode
- full-screen fill

It is intentionally lightweight for quick bring-up/testing.
"""

import time

try:
    from machine import Pin  # type: ignore[import-not-found]
except ImportError:  # Allows linting on desktop Python
    class Pin:  # type: ignore[override]
        OUT = 1

        def __init__(self, *args, **kwargs):
            pass

        def init(self, *args, **kwargs):
            pass

        def __call__(self, *args, **kwargs):
            pass

try:
    from micropython import const  # type: ignore[import-not-found]
except ImportError:  # Allows linting on desktop Python
    def const(value):
        return value


def _sleep_ms(ms):
    try:
        time.sleep_ms(ms)
    except AttributeError:  # Desktop Python compatibility
        time.sleep(ms / 1000)


_ST7735_SWRESET = const(0x01)
_ST7735_SLPOUT = const(0x11)
_ST7735_COLMOD = const(0x3A)
_ST7735_MADCTL = const(0x36)
_ST7735_CASET = const(0x2A)
_ST7735_RASET = const(0x2B)
_ST7735_INVON = const(0x21)
_ST7735_INVOFF = const(0x20)
_ST7735_NORON = const(0x13)
_ST7735_DISPON = const(0x29)
_ST7735_RAMWR = const(0x2C)


def color565(r, g, b):
    """Convert 8-bit RGB values to 16-bit RGB565."""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


class ST7735:
    def __init__(
        self,
        spi,
        cs,
        dc,
        rst,
        width=128,
        height=160,
        xstart=0,
        ystart=0,
        bgr=False,
        invert=False,
    ):
        self.spi = spi
        self.cs = cs
        self.dc = dc
        self.rst = rst
        self.width = width
        self.height = height
        self.xstart = xstart
        self.ystart = ystart
        self.bgr = bgr
        self.invert = invert

        # Ensure output mode
        if not isinstance(self.cs, Pin):
            raise TypeError("cs must be a machine.Pin")
        if not isinstance(self.dc, Pin):
            raise TypeError("dc must be a machine.Pin")
        if not isinstance(self.rst, Pin):
            raise TypeError("rst must be a machine.Pin")

        self.cs.init(Pin.OUT, value=1)
        self.dc.init(Pin.OUT, value=0)
        self.rst.init(Pin.OUT, value=1)

    def _write_cmd(self, cmd):
        self.cs(0)
        self.dc(0)
        self.spi.write(bytes([cmd]))
        self.cs(1)

    def _write_data(self, data):
        self.cs(0)
        self.dc(1)
        self.spi.write(data)
        self.cs(1)

    def _hard_reset(self):
        self.rst(1)
        _sleep_ms(20)
        self.rst(0)
        _sleep_ms(20)
        self.rst(1)
        _sleep_ms(150)

    def initr(self):
        """Initialize ST7735 (common 128x160 sequence)."""
        self._hard_reset()

        self._write_cmd(_ST7735_SWRESET)
        _sleep_ms(150)

        self._write_cmd(_ST7735_SLPOUT)
        _sleep_ms(255)

        self._write_cmd(_ST7735_COLMOD)
        self._write_data(b"\x05")  # 16-bit color
        _sleep_ms(10)

        self._write_cmd(_ST7735_MADCTL)
        madctl = 0xC0 | (0x08 if self.bgr else 0x00)
        self._write_data(bytes([madctl]))

        self._write_cmd(_ST7735_INVON if self.invert else _ST7735_INVOFF)
        _sleep_ms(10)

        self._write_cmd(_ST7735_NORON)
        _sleep_ms(10)

        self._write_cmd(_ST7735_DISPON)
        _sleep_ms(100)

    def _set_window(self, x0, y0, x1, y1):
        x0 += self.xstart
        x1 += self.xstart
        y0 += self.ystart
        y1 += self.ystart

        self._write_cmd(_ST7735_CASET)
        self._write_data(bytes([0x00, x0, 0x00, x1]))

        self._write_cmd(_ST7735_RASET)
        self._write_data(bytes([0x00, y0, 0x00, y1]))

        self._write_cmd(_ST7735_RAMWR)

    def fill(self, color):
        """Fill entire display with a single RGB565 color."""
        self._set_window(0, 0, self.width - 1, self.height - 1)

        hi = (color >> 8) & 0xFF
        lo = color & 0xFF

        # Send in chunks to keep memory usage low.
        chunk_pixels = 64
        chunk = bytes([hi, lo]) * chunk_pixels
        total_pixels = self.width * self.height

        self.cs(0)
        self.dc(1)

        while total_pixels > 0:
            count = chunk_pixels if total_pixels >= chunk_pixels else total_pixels
            if count == chunk_pixels:
                self.spi.write(chunk)
            else:
                self.spi.write(bytes([hi, lo]) * count)
            total_pixels -= count

        self.cs(1)
