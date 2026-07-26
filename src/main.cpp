#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "display.h"
#include "buttons.h"
#include "ventilator.h"
#include "wifi.h"
#include "settings.h"
#include "errors.h"
#include "utils.h"

// Global variables
static unsigned long last_debug_time = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=================================");
  Serial.println("Banya Microclimate Controller v1.0");
  Serial.println("=================================");
  Serial.println();
  
  // Initialize subsystems
  Serial.println("[INIT] Initializing subsystems...");
  
  settings_init();    // Load settings from EEPROM
  sensors_init();     // Initialize sensors (I2C, OneWire)
  display_init();     // Initialize display
  buttons_init();     // Initialize buttons
  ventilator_init();  // Initialize ventilator relay
  wifi_init();        // Initialize Wi-Fi and web server
  errors_init();      // Initialize error logger
  
  Serial.println("\n[INIT] All subsystems initialized successfully!");
  Serial.println("[INIT] Ready to operate.\n");
  
  display_show_main_screen();
}

void loop() {
  unsigned long now = millis();
  
  // Read sensors (every 1 second inside sensors_read)
  sensors_read();
  
  // Update button states
  buttons_update();
  
  // Update ventilator logic
  ventilator_update();
  
  // Update Wi-Fi state
  wifi_update();
  
  // Update settings (delayed save)
  settings_update();
  
  // Update display
  display_update();
  
  // Debug output every 10 seconds
  if (now - last_debug_time > 10000) {
    last_debug_time = now;
    debug_print_sensors();
  }
  
  delay(50);
}
