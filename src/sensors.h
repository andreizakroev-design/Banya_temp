#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>

typedef struct {
  float temp_top;           // Tверх
  float temp_bottom;        // Tниз
  float humidity_top;       // Hверх
  float humidity_bottom;    // Hниз
  float temp_avg;           // Tср
  float humidity_avg;       // Hср
  float temp_tank;          // Tбак
  float temp_change_rate;   // dT/dt in °C/min
  
  bool aht10_top_valid;     // Data validity flags
  bool aht10_bottom_valid;
  bool ds18b20_valid;
  
  unsigned long last_read_time;
} sensors_data_t;

void sensors_init();
void sensors_read();
float sensors_get_temp_change_rate();
sensors_data_t* sensors_get_data();

#endif // SENSORS_H
