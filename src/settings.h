#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>
#include "config.h"

typedef struct {
  uint16_t ventilator_time;    // Default: 30 minutes
  uint8_t stop_humidity;       // Default: 30%
  uint8_t crc8;                // Simple checksum
} settings_t;

void settings_init();
void settings_load();
void settings_save();
void settings_reset_to_defaults();
settings_t* settings_get();
void settings_set_ventilator_time(uint16_t minutes);
void settings_set_stop_humidity(uint8_t humidity);

#endif // SETTINGS_H
