#ifndef CONFIG_H
#define CONFIG_H

// ========== GPIO PINS ==========

// Display pins (SPI)
#define DISPLAY_CS    15   // D8
#define DISPLAY_DC    2    // D4
#define DISPLAY_CLK   14   // D5
#define DISPLAY_MOSI  13   // D7
#define DISPLAY_RST   -1   // Not used (connected to 3.3V)

// Sensors
#define I2C_SDA       4    // D2
#define I2C_SCL       5    // D1
#define DS18B20_PIN   12   // D6

// Buttons
#define MODE_PIN      3    // RX
#define START_PIN     1    // TX

// Ventilator relay
#define VENTILATOR_PIN 16  // D0

// ========== I2C ADDRESSES ==========
#define AHT10_ADDR_TOP    0x38  // Upper sensor
#define AHT10_ADDR_BOTTOM 0x39  // Lower sensor

// ========== SENSOR SETTINGS ==========
#define SENSOR_READ_INTERVAL   1000  // 1 second
#define DS18B20_RESOLUTION     12    // 12-bit resolution

// ========== VENTILATOR SETTINGS ==========
#define VENTILATOR_DEFAULT_TIME    30  // minutes
#define VENTILATOR_MIN_HUMIDITY    30  // percent
#define VENTILATOR_TIME_MIN        5   // minutes
#define VENTILATOR_TIME_MAX        120 // minutes
#define VENTILATOR_TIME_STEP       5   // minutes
#define HUMIDITY_MIN               10  // percent
#define HUMIDITY_MAX               90  // percent
#define HUMIDITY_STEP              5   // percent

// ========== DISPLAY SETTINGS ==========
#define DISPLAY_WIDTH             128
#define DISPLAY_HEIGHT            64
#define GRAPH_HISTORY_MINUTES     30
#define GRAPH_SAMPLE_INTERVAL     30  // seconds
#define GRAPH_MAX_POINTS          (GRAPH_HISTORY_MINUTES * 60 / GRAPH_SAMPLE_INTERVAL)

// ========== ERROR LOG ==========
#define ERROR_LOG_SIZE    10

// ========== MENU TIMEOUT ==========
#define MENU_TIMEOUT      10000  // milliseconds

// ========== BUTTON DEBOUNCE ==========
#define DEBOUNCE_TIME     50    // milliseconds
#define LONG_PRESS_TIME   3000  // milliseconds

// ========== EEPROM SETTINGS ==========
#define SETTINGS_ADDRESS  0

// ========== TEMPERATURE CALCULATION ==========
#define TEMP_HISTORY_TIME  10000  // 10 seconds for dT/dt calculation

// ========== WI-FI SETTINGS ==========
#define WIFI_SSID      "BanyaControl"
#define WIFI_IP        "192.168.4.1"
#define WIFI_GATEWAY   "192.168.4.1"
#define WIFI_SUBNET    "255.255.255.0"

#endif // CONFIG_H
