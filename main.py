from machine import Pin, SPI  # type: ignore[import-not-found]
import st7735
import time

# 1. Setup the SPI interface
spi = SPI(2, baudrate=20000000, polarity=0, phase=0, sck=Pin(18), mosi=Pin(23))

# 2. Setup the control pins
cs = Pin(5, Pin.OUT)
dc = Pin(2, Pin.OUT)
rst = Pin(4, Pin.OUT)

# 3. Initialize the Display
# If colors look swapped on your panel, change bgr to True.
# If colors look washed/odd, try invert=True.
display = st7735.ST7735(spi, cs, dc, rst, bgr=False, invert=False)
display.initr()

# 4. Test the screen colors
print("Starting RGB Color Cycle...")
colors = [
    st7735.color565(255, 0, 0),   # Red
    st7735.color565(0, 255, 0),   # Green
    st7735.color565(0, 0, 255)    # Blue
]

for color in colors:
    display.fill(color)
    time.sleep(1)

display.fill(0)
print("Screen test complete!")
