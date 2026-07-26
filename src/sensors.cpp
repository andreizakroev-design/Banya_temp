#include <Arduino.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"
#include "sensors.h"
#include "errors.h"

// Sensor objects
OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);

// Sensor data
sensors_data_t sensor_data = {0};

// Temperature history for dT/dt calculation
struct {
  float temp_avg;
  unsigned long timestamp;
} temp_history[2] = {{{0, 0}, {0, 0}}};

uint8_t history_index = 0;

// Function to read AHT10 sensor
bool aht10_read(uint8_t address, float &temperature, float &humidity) {
  // AHT10 initialization sequence
  Wire.beginTransmission(address);
  Wire.write(0xAC);  // Start measurement
  Wire.write(0x33);  // Parameter 1
  Wire.write(0x00);  // Parameter 2
  int error = Wire.endTransmission();
  
  if (error != 0) {
    return false;
  }
  
  delay(80);  // Wait for measurement to complete
  
  // Read 6 bytes of data
  Wire.requestFrom(address, (uint8_t)6);
  
  if (Wire.available() < 6) {
    return false;
  }
  
  uint8_t data[6];
  for (int i = 0; i < 6; i++) {
    data[i] = Wire.read();
  }
  
  // Check if measurement is ready (bit 7 of first byte should be 0)
  if (data[0] & 0x80) {
    return false;
  }
  
  // Extract humidity (20 bits from bytes 1-3)
  uint32_t humidity_raw = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | ((uint32_t)data[3] >> 4);
  humidity = (humidity_raw / 1048576.0f) * 100.0f;
  
  // Extract temperature (20 bits from bytes 3-5)
  uint32_t temp_raw = (((uint32_t)data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
  temperature = ((temp_raw / 1048576.0f) * 200.0f) - 50.0f;
  
  // Clamp values to reasonable ranges
  if (humidity > 100.0f) humidity = 100.0f;
  if (humidity < 0.0f) humidity = 0.0f;
  if (temperature > 125.0f || temperature < -40.0f) {
    return false;
  }
  
  return true;
}

// Initialize sensors
void sensors_init() {
  // Initialize I2C for AHT10 sensors
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);  // 100kHz for AHT10
  
  delay(100);
  
  // Initialize DS18B20
  ds18b20.begin();
  ds18b20.setResolution(DS18B20_RESOLUTION);
  ds18b20.requestTemperatures();
  
  sensor_data.last_read_time = millis();
  sensor_data.aht10_top_valid = false;
  sensor_data.aht10_bottom_valid = false;
  sensor_data.ds18b20_valid = false;
  
  Serial.println("[SENSORS] Initialized");
}

// Read all sensors
void sensors_read() {
  unsigned long now = millis();
  
  // Read sensors every SENSOR_READ_INTERVAL milliseconds
  if (now - sensor_data.last_read_time < SENSOR_READ_INTERVAL) {
    return;
  }
  
  sensor_data.last_read_time = now;
  
  // Read AHT10 Top sensor (address 0x38)
  if (!aht10_read(AHT10_ADDR_TOP, sensor_data.temp_top, sensor_data.humidity_top)) {
    if (sensor_data.aht10_top_valid) {
      // Connection lost
      errors_log(ERROR_AHT10_TOP_DISCONNECTED, "AHT10 Top", "No connection");
    }
    sensor_data.aht10_top_valid = false;
  } else {
    sensor_data.aht10_top_valid = true;
  }
  
  // Read AHT10 Bottom sensor (address 0x39)
  if (!aht10_read(AHT10_ADDR_BOTTOM, sensor_data.temp_bottom, sensor_data.humidity_bottom)) {
    if (sensor_data.aht10_bottom_valid) {
      // Connection lost
      errors_log(ERROR_AHT10_BOTTOM_DISCONNECTED, "AHT10 Bottom", "No connection");
    }
    sensor_data.aht10_bottom_valid = false;
  } else {
    sensor_data.aht10_bottom_valid = true;
  }
  
  // Read DS18B20
  float temp_tank = ds18b20.getTempCByIndex(0);
  
  if (temp_tank == DEVICE_DISCONNECTED_C) {
    if (sensor_data.ds18b20_valid) {
      // Connection lost
      errors_log(ERROR_DS18B20_DISCONNECTED, "DS18B20", "No connection");
    }
    sensor_data.ds18b20_valid = false;
  } else {
    sensor_data.temp_tank = temp_tank;
    sensor_data.ds18b20_valid = true;
  }
  
  // Request new temperature conversion
  ds18b20.requestTemperatures();
  
  // Calculate average temperature and humidity
  if (sensor_data.aht10_top_valid && sensor_data.aht10_bottom_valid) {
    sensor_data.temp_avg = (sensor_data.temp_top + sensor_data.temp_bottom) / 2.0f;
    sensor_data.humidity_avg = (sensor_data.humidity_top + sensor_data.humidity_bottom) / 2.0f;
  } else if (sensor_data.aht10_top_valid) {
    sensor_data.temp_avg = sensor_data.temp_top;
    sensor_data.humidity_avg = sensor_data.humidity_top;
  } else if (sensor_data.aht10_bottom_valid) {
    sensor_data.temp_avg = sensor_data.temp_bottom;
    sensor_data.humidity_avg = sensor_data.humidity_bottom;
  }
  
  // Update temperature history for dT/dt calculation
  history_index = (history_index + 1) % 2;
  temp_history[history_index].temp_avg = sensor_data.temp_avg;
  temp_history[history_index].timestamp = now;
}

// Get temperature change rate (dT/dt in °C/min)
float sensors_get_temp_change_rate() {
  if (!sensor_data.aht10_top_valid && !sensor_data.aht10_bottom_valid) {
    return 0.0f;
  }
  
  // Need at least 10 seconds of history
  unsigned long time_diff = temp_history[1 - history_index].timestamp - temp_history[history_index].timestamp;
  
  if (time_diff < TEMP_HISTORY_TIME) {
    return 0.0f;
  }
  
  float temp_diff = temp_history[1 - history_index].temp_avg - temp_history[history_index].temp_avg;
  
  // Convert to °C/min: dT/dt = (dT / dt_ms) * 60000 ms/min
  float change_rate = (temp_diff / (float)time_diff) * 60000.0f;
  
  return change_rate;
}

// Get sensor data structure
sensors_data_t* sensors_get_data() {
  return &sensor_data;
}
