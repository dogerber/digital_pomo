#define USER_SETUP_INFO "Setup for LilyGO T-Display-S3"

// Define the display type
#define ILI9341_DRIVER

// Define the pin connections
#define TFT_CS 10 // Chip select control pin
#define TFT_DC 9  // Data Command control pin
#define TFT_RST 5 // Reset pin (could connect to RST pin)

// Define the SPI communication settings
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_MISO 19

// Define the backlight control pin
#define TFT_BL 4 // LED back-light control pin

// Touchscreen pins
#define TOUCH_CS 21
#define TOUCH_IRQ 22

// Include touch functionality
#define TOUCH_SUPPORT
#define BOARD_I2C_SCL 17
#define BOARD_I2C_SDA 18
#define BOARD_TOUCH_IRQ 16
#define BOARD_TOUCH_RST 21

// Other settings
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 20000000
