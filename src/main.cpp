#include <Arduino.h>
#include "esp_camera.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "pins.h"

// Enable/disable noisy serial output (comment out to disable)
#define DEBUG

// OLED Constants
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SDA           15
#define SCL           14

// Define FPS: affects (ideal) delay time between each frame
#define FPS           16

// SSD1306 display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup()
{
  #ifdef DEBUG
    Serial.begin(115200);
  #endif

  // Initialize display
  Wire.begin(SDA, SCL); // Required if not using the 'default' SDA/SCL I2C interface pins
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { 
    #ifdef DEBUG
      Serial.println(F("SSD1306 allocation failed"));
    #endif
    for(;;); // Don't proceed, loop forever
  }
  else
  {
    #ifdef DEBUG
      Serial.println("SSD1306 initialized.");
    #endif
  }
  display.display();
  delay(100);
  
  // Initialize OV2640
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_GRAYSCALE; // Only stores luminance values, ideal for the SSD1306
  config.frame_size = FRAMESIZE_QVGA; // 320 x 240, smallest possible frame size
  config.jpeg_quality = 10;
  config.fb_count = 2;
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    #ifdef DEBUG
      Serial.printf("Camera init failed with error 0x%x", err);
    #endif
    for(;;);
  }
  else
  {
    #ifdef DEBUG
      Serial.println("Camera initialized.");
    #endif
  }

  // Grab test frame from camera
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  
  if(!fb)
  {
    #ifdef DEBUG
      Serial.println("Initial camera capture failed");
    #endif
  }
  else
  {
    #ifdef DEBUG
      Serial.println("Initial camera capture successful.");
    #endif
  }
  esp_camera_fb_return(fb); // Free the memory where framebuffer is stored
  
  #ifdef DEBUG
    Serial.println("Initializing mirroring...");
  #endif
  delay(1000);
}

void loop()
{
  // Frametime counter
  unsigned long zeroTime = millis();

  // Get new frame from camera
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();

  if(!fb)
  {
    #ifdef DEBUG
      Serial.println("Camera capture failed");
    #endif
  }
  else
  {
    // Downscale using nearest-neighbor interpolation and quantize based on brightness values
    bool *resized_image = new bool[128 * 64]; // Boolean output buffer to store the resized image as binary luminance values
    uint8_t *input_image = fb->buf; // Pointer to the framebuffer (grayscale pixel data, luminance values as uint8_t array)
    uint8_t y_drop = 16; // Crop vertically by 16 pixels to fit image in the 128x64 screen
    // Iterate over each pixel in the output image
    for (uint32_t y = y_drop; y < y_drop + 64; y++)
    {
      uint32_t src_y = (uint32_t)(y * 2.5); // Find the nearest neighboring row in the input image
      for (uint32_t x = 0; x < 128; x++)
      {
        uint32_t src_x = (uint32_t)(x * 2.5); // Find the nearest neighboring column in the input image, and thus the NN-pixel
        
        // Copy the nearest neighbor pixel from the input to the output, while quantizing (~0.47 of max brightness as quantization point)
        resized_image[(y - y_drop) * 128 + x] = (input_image[src_y * 320 + src_x] >= 120);
      }
    }

    // Free camera framebuffer memory
    esp_camera_fb_return(fb);

    // Process image to SSD1306 compatible bitmap (groups of 8 horizontal pixel values stored as uint8_t values equivalent to their representation in an 8-bit unsigned binary number)
    unsigned char *image_bmp = new unsigned char[16 * 64];
    // Iterate over each data group in the output bitmap
    for (int y = 0; y < 64; y++)
    {
      for (int x_g = 0; x_g < 16; x_g++)
      {
        uint8_t byte_val = 0; // Empty 8 bit unsigned integer
        // Assign bits from the resized image luminance booleans to the output bitmap group in MSB-first order
        for (int i = 0; i < 8; i++)
        {
          // OR-assignment of left-shifted boolean values to the unsigned integer
          byte_val |= (resized_image[(y * 128) + (x_g * 8 + i)] << (7 - i));
        }
        // Store unsigned 8-bit integer value in the char array
        image_bmp[(y * 16) + x_g] = byte_val;
      }
    }
    delete[] resized_image; // Free heap memory

    // Push image to SSD1306
    display.clearDisplay();
    display.drawBitmap(0, 0, image_bmp, 128, 64, 1);
    display.display();

    delete[] image_bmp; // Free heap memory
  }

  #ifdef DEBUG
    Serial.print("FPS: "); Serial.println((uint64_t)(1000/(millis() - zeroTime)));
  #endif
  // Delay for remainder of time according to FPS spec
  delay((1000/FPS) - ((millis() - zeroTime) < (1000/FPS))*(millis() - zeroTime));
}