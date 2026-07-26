#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>

// Debug output
void debug_print_sensors();
void debug_print(const char* format, ...);

// Temperature averaging
float calculate_average(float val1, float val2, bool val1_valid, bool val2_valid);

// CRC8 calculation for settings
uint8_t crc8(uint8_t* data, uint8_t length);

#endif // UTILS_H
