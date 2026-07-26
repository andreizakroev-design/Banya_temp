#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "display.h"
#include "buttons.h"
#include "ventilator.h"
#include "wifi.h"
#include "settings.h"
#include "errors.h"

// Global variables
sensors_data_t sensor_data;
system_state_t system_state;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\nBanya Controller v1.0 starting...");
  
  // Initialize settings from EEPROM
  settings_init();
  
  // Initialize sensors
  sensors_init();
  
  // Initialize display
  display_init();
  
  // Initialize buttons
  buttons_init();
  
  // Initialize ventilator
  ventilator_init();
  
  // Initialize Wi-Fi
  wifi_init();
  
  // Initialize error log
  errors_init();
  
  // Initialize system state
  system_state.screen = 0;
  system_state.menu_mode = false;
  system_state.menu_item = 0;
  system_state.ventilator_running = false;
  system_state.ventilator_end_time = 0;
  
  Serial.println("Banya Controller initialized successfully!");
  display_show_main_screen();
}

void loop() {
  // Read sensors
  sensors_read();
  
  // Handle buttons
  buttons_update();
  
  // Update ventilator logic
  ventilator_update();
  
  // Update display
  display_update();
  
  // Debug output every 10 seconds
  static unsigned long last_debug = 0;
  if (millis() - last_debug > 10000) {
    last_debug = millis();
    debug_print_sensors();
  }
  
  delay(50);
}
