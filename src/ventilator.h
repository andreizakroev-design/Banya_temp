#ifndef VENTILATOR_H
#define VENTILATOR_H

#include <stdint.h>

typedef struct {
  bool running;
  unsigned long end_time;  // milliseconds when ventilator should stop
  uint16_t ventilator_time;  // minutes
  uint8_t stop_humidity;   // percent
} ventilator_state_t;

void ventilator_init();
void ventilator_update();
void ventilator_start();
void ventilator_stop();
bool ventilator_is_running();
unsigned long ventilator_get_remaining_time();
void ventilator_set_time(uint16_t minutes);
void ventilator_set_stop_humidity(uint8_t humidity);

#endif // VENTILATOR_H
