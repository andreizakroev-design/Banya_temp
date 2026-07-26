#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>

typedef enum {
  BUTTON_NONE,
  BUTTON_SHORT_PRESS,
  BUTTON_LONG_PRESS
} button_event_t;

void buttons_init();
void buttons_update();
button_event_t buttons_get_mode_event();
button_event_t buttons_get_start_event();

#endif // BUTTONS_H
