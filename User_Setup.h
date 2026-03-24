// User_Setup.h - TFT_eSPI Configuration for ESP32-C3

// Display setup
#define ILI9341_DRIVER
#define TFT_WIDTH 240
#define TFT_HEIGHT 320

// ESP32 SPI pins - corrected for ESP32-C3
#define TFT_MOSI 7 // GPIO7
#define TFT_SCLK 6 // GPIO6
#define TFT_CS 10  // GPIO10
#define TFT_DC 9   // GPIO9
#define TFT_RST 8  // GPIO8

// Color depth
#define TFT_BL 21            // Backlight pin
#define TFT_BACKLIGHT_ON LOW // Active low

// SPI frequency
#define SPI_FREQUENCY 80000000

// Font configuration - DISABLE SMOOTH FONTS TO AVOID SERIAL DEPENDENCY
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT 0

// Use default SPI for ESP32
#define USE_SPI_PORT 1

// No Anti-aliasing for fonts
#define FONT_ANTIALIASING 0

#endif // USER_SETUP_H
