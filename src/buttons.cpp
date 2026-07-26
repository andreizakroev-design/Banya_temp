#include <Arduino.h>
#include "config.h"
#include "buttons.h"
#include "display.h"
#include "ventilator.h"
#include "settings.h"

// Button state structures
struct {
    uint8_t pin;
    uint16_t debounce_time;
    bool last_state;
    unsigned long press_start_time;
    bool long_press_detected;
    button_event_t current_event;
} mode_button = {MODE_PIN, DEBOUNCE_TIME, HIGH, 0, false, BUTTON_NONE};

struct {
    uint8_t pin;
    uint16_t debounce_time;
    bool last_state;
    unsigned long press_start_time;
    bool long_press_detected;
    button_event_t current_event;
} start_button = {START_PIN, DEBOUNCE_TIME, HIGH, 0, false, BUTTON_NONE};

// Initialize buttons
void buttons_init() {
    pinMode(mode_button.pin, INPUT_PULLUP);
    pinMode(start_button.pin, INPUT_PULLUP);
    
    Serial.println("[BUTTONS] Initialized");
}

// Helper function to process button
static void process_button(struct {
    uint8_t pin;
    uint16_t debounce_time;
    bool last_state;
    unsigned long press_start_time;
    bool long_press_detected;
    button_event_t current_event;
} *btn) {
    bool current_state = digitalRead(btn->pin);
    unsigned long now = millis();
    
    // Debounce
    if (current_state != btn->last_state) {
        btn->last_state = current_state;
        btn->debounce_time = DEBOUNCE_TIME;
        return;
    }
    
    if (btn->debounce_time > 0) {
        btn->debounce_time--;
        return;
    }
    
    // Button press detected (LOW is pressed due to INPUT_PULLUP)
    if (current_state == LOW) {
        if (btn->press_start_time == 0) {
            btn->press_start_time = now;
            btn->long_press_detected = false;
            btn->current_event = BUTTON_NONE;
        }
        
        // Check for long press
        if (!btn->long_press_detected && (now - btn->press_start_time) >= LONG_PRESS_TIME) {
            btn->long_press_detected = true;
            btn->current_event = BUTTON_LONG_PRESS;
        }
    } else {
        // Button released
        if (btn->press_start_time > 0) {
            if (!btn->long_press_detected) {
                btn->current_event = BUTTON_SHORT_PRESS;
            }
            btn->press_start_time = 0;
        }
    }
}

// Update buttons
void buttons_update() {
    process_button(&mode_button);
    process_button(&start_button);
    
    // Handle MODE button
    if (mode_button.current_event != BUTTON_NONE) {
        if (mode_button.current_event == BUTTON_SHORT_PRESS) {
            // Short press: next screen or menu item
            display_next_screen();
            Serial.println("[BUTTONS] MODE short press");
        } else if (mode_button.current_event == BUTTON_LONG_PRESS) {
            // Long press: enter menu
            display_enter_menu();
            Serial.println("[BUTTONS] MODE long press - menu entered");
        }
        mode_button.current_event = BUTTON_NONE;
    }
    
    // Handle START button
    if (start_button.current_event != BUTTON_NONE) {
        if (start_button.current_event == BUTTON_SHORT_PRESS) {
            if (display_is_in_menu()) {
                // In menu: change parameter
                settings_t* settings = settings_get();
                uint8_t menu_item = display_get_menu_item();
                
                if (menu_item == 0) {
                    // Ventilator time
                    uint16_t new_time = settings->ventilator_time + VENTILATOR_TIME_STEP;
                    if (new_time > VENTILATOR_TIME_MAX) {
                        new_time = VENTILATOR_TIME_MIN;
                    }
                    settings_set_ventilator_time(new_time);
                    Serial.print("[BUTTONS] Ventilator time changed to ");
                    Serial.print(new_time);
                    Serial.println(" minutes");
                } else if (menu_item == 1) {
                    // Stop humidity
                    uint8_t new_humidity = settings->stop_humidity + HUMIDITY_STEP;
                    if (new_humidity > HUMIDITY_MAX) {
                        new_humidity = HUMIDITY_MIN;
                    }
                    settings_set_stop_humidity(new_humidity);
                    Serial.print("[BUTTONS] Stop humidity changed to ");
                    Serial.print(new_humidity);
                    Serial.println("%");
                }
            } else {
                // Outside menu: toggle ventilator
                if (ventilator_is_running()) {
                    ventilator_stop();
                    Serial.println("[BUTTONS] Ventilator stopped");
                } else {
                    ventilator_start();
                    Serial.println("[BUTTONS] Ventilator started");
                }
            }
        }
        start_button.current_event = BUTTON_NONE;
    }
}

// Get button events
button_event_t buttons_get_mode_event() {
    return mode_button.current_event;
}

button_event_t buttons_get_start_event() {
    return start_button.current_event;
}
