# ESP32 CAM2OLED
Program to mirror output from an OV2640 module on an ESP32-CAM to an SSD1306 OLED display.

## Demo
![Image](https://github.com/user-attachments/assets/46adc268-da5c-4c07-9dc4-88d6a976b4d4)

## Functional Description
The program takes the smallest possible grayscale image from an OV2640 on an ESP32-CAM (QVGA: 320x420), then uses nearest-neighbor interpolation and linear quantization to downscale the image to a monochrome 128x64 bitmap which is finally displayed on an SSD1306 OLED display connected via an I2C interface.
### Performance
FPS: 14-26 (Downscaling: nearest-neighbor, Quantization: simple/linear)
### Possible improvements
- Use of the [U8g2 library](https://github.com/olikraus/u8g2) to potentially speed up SSD1306 output.
- Use of dithering to get a more detailed monochrome image from very bright/dark inputs
- Use of bilinear downsampling instead of nearest-neighbour, preserves more detail
- Incorporating multitasking to speed up downsampling/quantization.

Note: As far as I understand, the primary bottleneck in the current program is the SSD1306/GFX library, which holds up the main loop for quite a bit in order to push data to the OLED display. Technically, the U8g2 library (which I believe is faster than the Adafruit libraries) presents the biggest potential for lowering frametimes. Dual-core processing on the other hand, while tempting, requires far more effort and can really only speed up the image array processing, which is not the primary bottleneck in the current case. However, perhaps with dithering and/or more elaborate downsampling, it would provide a noticeable improvement.

## Usage Instructions
Tested with: AI Thinker ESP32-CAM (OV2640), Adafruit SSD1306 OLED Display (128x64). For any other ESP32 dev boards or a different display size, the program would need to be modified.
1. Clone this repository, open it with PlatformIO in VS Code.
2. Add the Adafruit_SSD1306 and Adafruit_GFX libraries using the library manager.
3. Change the SCL and SDA pin definitions in main.cpp to the pins you want to use for the SSD1306.
4. Compile and upload the program.

## Credits
Great write-up about [Quantization/Dithering](https://surma.dev/things/ditherpunk/)
[ESP32 Camera Driver](https://github.com/espressif/esp32-camera/tree/master)
[Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
[Adafruit SSD1306 Library](https://github.com/adafruit/Adafruit_SSD1306)