#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"
#include "sensors.h"

void display_init();
void display_update();
void display_show_main_screen();
void display_next_screen();
void display_enter_menu();
void display_exit_menu();
bool display_is_in_menu();
uint8_t display_get_menu_item();
uint8_t display_get_current_screen();

#endif // DISPLAY_H
