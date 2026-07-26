#ifndef ERRORS_H
#define ERRORS_H

#include <stdint.h>
#include "config.h"

typedef enum {
  ERROR_AHT10_TOP_DISCONNECTED,
  ERROR_AHT10_BOTTOM_DISCONNECTED,
  ERROR_DS18B20_DISCONNECTED,
  ERROR_DISPLAY_FAILURE
} error_type_t;

typedef struct {
  unsigned long time_minutes;  // Time from startup
  error_type_t type;
  const char* source;
  const char* message;
} error_entry_t;

void errors_init();
void errors_log(error_type_t type, const char* source, const char* message);
error_entry_t* errors_get_log();
uint8_t errors_get_count();
void errors_clear();

#endif // ERRORS_H
