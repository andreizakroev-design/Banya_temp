#include <Arduino.h>
#include "config.h"
#include "errors.h"

// Error log
error_entry_t error_log[ERROR_LOG_SIZE];
uint8_t error_count = 0;
unsigned long startup_time = 0;

void errors_init() {
  error_count = 0;
  startup_time = millis();
  Serial.println("[ERRORS] Error logger initialized");
}

void errors_log(error_type_t type, const char* source, const char* message) {
  // Find the source of error
  const char* error_source = source;
  const char* error_message = message;
  
  // Create new error entry
  error_entry_t entry;
  entry.time_minutes = (millis() - startup_time) / 60000;
  entry.type = type;
  entry.source = error_source;
  entry.message = error_message;
  
  // Add to log
  if (error_count < ERROR_LOG_SIZE) {
    error_log[error_count] = entry;
    error_count++;
  } else {
    // Shift entries and add new one at the end
    for (int i = 0; i < ERROR_LOG_SIZE - 1; i++) {
      error_log[i] = error_log[i + 1];
    }
    error_log[ERROR_LOG_SIZE - 1] = entry;
  }
  
  // Print to serial
  Serial.print("[ERROR] (");
  Serial.print(entry.time_minutes);
  Serial.print(" min) ");
  Serial.print(entry.source);
  Serial.print(" - ");
  Serial.println(entry.message);
}

error_entry_t* errors_get_log() {
  return error_log;
}

uint8_t errors_get_count() {
  return error_count;
}

void errors_clear() {
  error_count = 0;
  Serial.println("[ERRORS] Log cleared");
}
