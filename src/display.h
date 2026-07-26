#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"
#include "sensors.h"

void display_init();
void display_update();
void display_show_main_screen();
void display_show_temp_rate_screen();
void display_show_graph_screen();
void display_show_error_log_screen();
void display_show_menu_ventilator_time();
void display_show_menu_humidity();
void display_show_menu_wifi_info();

#endif // DISPLAY_H
