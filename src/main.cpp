#include <Arduino.h>
#include "esp_camera.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1
#define SDA           15
#define SCL           14

#define FPS           16

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LOGO_HEIGHT   4
#define LOGO_WIDTH    16
static const unsigned char PROGMEM logo_bmp[] =
{ B10000000, B00000001,
  B10000001, B10000001,
  B10000000, B00000001,
  B10000001, B10000001 };

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void setup()
{
  Serial.begin(115200);

  Wire.begin(SDA, SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  else
  {
    Serial.println("SSD1306 initialized.");
  }
  display.display();
  delay(100);
  //display.drawBitmap(1, 1, logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, 1);
  //display.display();
  
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
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.frame_size = FRAMESIZE_QVGA; // 320 x 240
  config.jpeg_quality = 10;
  config.fb_count = 2;
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  else
  {
    Serial.println("Camera initialized.");
  }
  camera_fb_t * fb = NULL;
  
  fb = esp_camera_fb_get();  
  if(!fb)
  {
    Serial.println("Initial camera capture failed");
    return;
  }
  else
  {
    Serial.println("Initial camera capture successful.");
  }
  esp_camera_fb_return(fb); // Free the memory where fb is stored
  
  delay(1000);
}

void loop()
{
  unsigned long zeroTime = millis();
  // Get image from camera
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if(!fb)
  {
    Serial.println("Camera capture failed");
  }
  else
  {
    Serial.println("Capture successful.");
    // Downscale using nearest-neighbor interpolation, quantize based on individual brightness values
    // Output buffer to store the resized image
    bool *resized_image = new bool[128 * 64];
    // Pointer to the input framebuffer (grayscale pixel data)
    uint8_t *input_image = fb->buf;
    // Iterate over each pixel in the output image
    uint8_t y_drop = 16;  // Crop vertically by 16 pixels each end
    for (uint64_t y = y_drop; y < y_drop + 64; y++)
    {
      // Find the corresponding row in the input image
      uint64_t src_y = (uint64_t)(y * 2.5);
      for (uint64_t x = 0; x < 128; x++)
      {
        // Find the corresponding column in the input image
        uint64_t src_x = (uint64_t)(x * 2.5);
        // Copy the nearest pixel from the input to the output, while quantizing based on brightness values
        resized_image[(y - y_drop) * 128 + x] = (input_image[src_y * 320 + src_x] >= 120);
      }
    }
    Serial.println("Made it past downsampler.");

    // Free memory
    esp_camera_fb_return(fb);

    // Process image to SSD1306 compatible bitmap
    unsigned char *image_bmp = new unsigned char[128 * 8];
    for (int y = 0; y < 64; y++)
    {
      for (int x_g = 0; x_g < 16; x_g++)
      {
        uint8_t byte_val = 0;
        for (int i = 0; i < 8; i++)
        {
          // Assign bits in MSB-first order
          byte_val |= (resized_image[(y * 128) + (x_g * 8 + i)] << (7 - i));
        }
        image_bmp[y * 16 + x_g] = byte_val;
      }
    }
    delete[] resized_image; // Free heap memory
    Serial.println("Made it past bitmap code.");

    // Push image to SSD1306
    display.clearDisplay();
    display.drawBitmap(0, 0, image_bmp, 128, 64, 1);
    display.display();
    delete[] image_bmp; // Free heap memory
    Serial.println("Made it past display code.");
  }

  Serial.print("Took "); Serial.print((millis() - zeroTime)); Serial.println(" ms to do a frame.");
  // Delay for remainder of time according to FPS spec
  delay((uint64_t)1000/FPS);
}