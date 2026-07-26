#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"
#include "settings.h"
#include "utils.h"

// Global settings
settings_t current_settings = {0};

// Flag for pending save
bool settings_pending_save = false;
unsigned long settings_last_change_time = 0;

// Initialize settings
void settings_init() {
  EEPROM.begin(512);
  settings_load();
  Serial.println("[SETTINGS] Initialized");
}

// Calculate CRC8 checksum
static uint8_t calculate_crc8(settings_t* data) {
  uint8_t* ptr = (uint8_t*)data;
  uint8_t crc = 0;
  
  // Calculate CRC for all bytes except the CRC byte itself
  for (int i = 0; i < sizeof(settings_t) - 1; i++) {
    crc = crc8_byte(crc, ptr[i]);
  }
  
  return crc;
}

// CRC8 byte calculation
static uint8_t crc8_byte(uint8_t crc, uint8_t data) {
  crc ^= data;
  for (int i = 0; i < 8; i++) {
    if (crc & 0x80) {
      crc = (crc << 1) ^ 0x07;
    } else {
      crc = crc << 1;
    }
  }
  return crc;
}

// Load settings from EEPROM
void settings_load() {
  // Read settings from EEPROM
  EEPROM.get(SETTINGS_ADDRESS, current_settings);
  
  // Verify CRC
  uint8_t calculated_crc = calculate_crc8(&current_settings);
  
  if (calculated_crc != current_settings.crc8) {
    // CRC mismatch - settings are corrupted
    Serial.println("[SETTINGS] CRC mismatch, resetting to defaults");
    settings_reset_to_defaults();
  } else {
    Serial.print("[SETTINGS] Loaded - Ventilator time: ");
    Serial.print(current_settings.ventilator_time);
    Serial.print(" min, Stop humidity: ");
    Serial.print(current_settings.stop_humidity);
    Serial.println("%");
  }
}

// Save settings to EEPROM
void settings_save() {
  if (!settings_pending_save) {
    return;
  }
  
  // Calculate CRC
  current_settings.crc8 = calculate_crc8(&current_settings);
  
  // Write to EEPROM
  EEPROM.put(SETTINGS_ADDRESS, current_settings);
  EEPROM.commit();
  
  settings_pending_save = false;
  
  Serial.print("[SETTINGS] Saved - Ventilator time: ");
  Serial.print(current_settings.ventilator_time);
  Serial.print(" min, Stop humidity: ");
  Serial.print(current_settings.stop_humidity);
  Serial.println("%");
}

// Reset to default values
void settings_reset_to_defaults() {
  current_settings.ventilator_time = VENTILATOR_DEFAULT_TIME;
  current_settings.stop_humidity = VENTILATOR_MIN_HUMIDITY;
  current_settings.crc8 = calculate_crc8(&current_settings);
  
  EEPROM.put(SETTINGS_ADDRESS, current_settings);
  EEPROM.commit();
  
  Serial.println("[SETTINGS] Reset to defaults");
}

// Get settings pointer
settings_t* settings_get() {
  return &current_settings;
}

// Set ventilator time
void settings_set_ventilator_time(uint16_t minutes) {
  if (minutes < VENTILATOR_TIME_MIN || minutes > VENTILATOR_TIME_MAX) {
    return;
  }
  
  if (current_settings.ventilator_time != minutes) {
    current_settings.ventilator_time = minutes;
    settings_pending_save = true;
    settings_last_change_time = millis();
  }
}

// Set stop humidity threshold
void settings_set_stop_humidity(uint8_t humidity) {
  if (humidity < HUMIDITY_MIN || humidity > HUMIDITY_MAX) {
    return;
  }
  
  if (current_settings.stop_humidity != humidity) {
    current_settings.stop_humidity = humidity;
    settings_pending_save = true;
    settings_last_change_time = millis();
  }
}

// Update - handles delayed save to Flash
void settings_update() {
  // Save to Flash 5 seconds after last change
  if (settings_pending_save && 
      (millis() - settings_last_change_time > 5000)) {
    settings_save();
  }
}
