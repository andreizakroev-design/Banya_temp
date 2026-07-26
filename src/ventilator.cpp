#include <Arduino.h>
#include "config.h"
#include "ventilator.h"
#include "sensors.h"
#include "settings.h"

// Ventilator state
static struct {
    bool running;
    unsigned long end_time;
    uint16_t time_minutes;
    uint8_t stop_humidity;
} ventilator_state = {false, 0, VENTILATOR_DEFAULT_TIME, VENTILATOR_MIN_HUMIDITY};

// Initialize ventilator
void ventilator_init() {
    pinMode(VENTILATOR_PIN, OUTPUT);
    digitalWrite(VENTILATOR_PIN, LOW);  // Relay off (active HIGH)
    
    // Load settings
    settings_t* settings = settings_get();
    ventilator_state.time_minutes = settings->ventilator_time;
    ventilator_state.stop_humidity = settings->stop_humidity;
    
    Serial.println("[VENTILATOR] Initialized");
}

// Start ventilator
void ventilator_start() {
    if (ventilator_state.running) {
        return;
    }
    
    ventilator_state.running = true;
    ventilator_state.end_time = millis() + (ventilator_state.time_minutes * 60000UL);
    
    digitalWrite(VENTILATOR_PIN, HIGH);  // Relay on
    
    Serial.print("[VENTILATOR] Started for ");
    Serial.print(ventilator_state.time_minutes);
    Serial.println(" minutes");
}

// Stop ventilator
void ventilator_stop() {
    if (!ventilator_state.running) {
        return;
    }
    
    ventilator_state.running = false;
    digitalWrite(VENTILATOR_PIN, LOW);  // Relay off
    
    Serial.println("[VENTILATOR] Stopped");
}

// Update ventilator logic
void ventilator_update() {
    if (!ventilator_state.running) {
        return;
    }
    
    sensors_data_t* data = sensors_get_data();
    unsigned long now = millis();
    
    // Condition 1: Time expired
    if (now >= ventilator_state.end_time) {
        Serial.println("[VENTILATOR] Time expired");
        ventilator_stop();
        return;
    }
    
    // Condition 2: Humidity below threshold
    if (data->aht10_top_valid || data->aht10_bottom_valid) {
        if (data->humidity_avg <= ventilator_state.stop_humidity) {
            Serial.print("[VENTILATOR] Humidity threshold reached: ");
            Serial.print(data->humidity_avg);
            Serial.print(" <= ");
            Serial.println(ventilator_state.stop_humidity);
            ventilator_stop();
            return;
        }
    }
}

// Check if ventilator is running
bool ventilator_is_running() {
    return ventilator_state.running;
}

// Get remaining time in seconds
unsigned long ventilator_get_remaining_time() {
    if (!ventilator_state.running) {
        return 0;
    }
    
    unsigned long now = millis();
    if (now >= ventilator_state.end_time) {
        return 0;
    }
    
    return (ventilator_state.end_time - now) / 1000;
}

// Set ventilator time
void ventilator_set_time(uint16_t minutes) {
    if (minutes >= VENTILATOR_TIME_MIN && minutes <= VENTILATOR_TIME_MAX) {
        ventilator_state.time_minutes = minutes;
        if (ventilator_state.running) {
            ventilator_state.end_time = millis() + (minutes * 60000UL);
        }
    }
}

// Set stop humidity
void ventilator_set_stop_humidity(uint8_t humidity) {
    if (humidity >= HUMIDITY_MIN && humidity <= HUMIDITY_MAX) {
        ventilator_state.stop_humidity = humidity;
    }
}
