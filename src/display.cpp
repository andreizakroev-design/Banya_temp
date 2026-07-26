#include <Arduino.h>
#include <U8g2lib.h>
#include "config.h"
#include "display.h"
#include "sensors.h"
#include "ventilator.h"
#include "settings.h"
#include "errors.h"

// Display object - Software SPI
U8G2_ST7565_ERC12864_ALT_1_4W_SW_SPI u8g2(
    U8G2_R0,
    DISPLAY_CLK,   // SCL - GPIO14 (D5)
    DISPLAY_MOSI,  // SI - GPIO13 (D7)
    DISPLAY_CS,    // CS - GPIO15 (D8)
    DISPLAY_DC,    // RS (DC) - GPIO2 (D4)
    U8X8_PIN_NONE  // RST - not used
);

// Display state variables
static uint8_t current_screen = 0;
static bool menu_mode = false;
static uint8_t menu_item = 0;
static unsigned long menu_timeout = 0;
static unsigned long last_graph_update = 0;

// Graph data structures
static struct {
    float temp[GRAPH_MAX_POINTS];
    float humidity[GRAPH_MAX_POINTS];
    uint8_t index;
    uint8_t count;
    unsigned long last_sample_time;
} graph_data = {0};

// Initialize display
void display_init() {
    u8g2.begin();
    u8g2.setFont(u8g2_font_ncenB08_tr);  // Medium font
    u8g2.setFontRefHeightExtendedText();
    u8g2.setDrawColor(1);
    u8g2.setFontPerspectiveMode(0);
    u8g2.setBitmapMode(0);
    
    // Clear display
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.drawStr(10, 30, "Banya");
    u8g2.drawStr(10, 50, "Initializing...");
    u8g2.sendBuffer();
    
    delay(500);
    
    // Initialize graph data
    graph_data.index = 0;
    graph_data.count = 0;
    graph_data.last_sample_time = millis();
    
    Serial.println("[DISPLAY] Initialized");
}

// Draw main screen (Screen 0)
static void display_draw_main_screen() {
    sensors_data_t* data = sensors_get_data();
    
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    
    // Top row: T↑ and T↓
    u8g2.drawStr(0, 10, "T↑");
    if (data->aht10_top_valid) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", data->temp_top);
        u8g2.drawStr(15, 10, buf);
    } else {
        u8g2.drawStr(15, 10, "ERR");
    }
    
    u8g2.drawStr(72, 10, "T↓");
    if (data->aht10_bottom_valid) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", data->temp_bottom);
        u8g2.drawStr(87, 10, buf);
    } else {
        u8g2.drawStr(87, 10, "ERR");
    }
    
    // Middle section: Average temperature (large)
    u8g2.setFont(u8g2_font_ncenB24_tr);
    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "%.1f", data->temp_avg);
    uint8_t width = u8g2.getStrWidth(temp_str);
    u8g2.drawStr((128 - width) / 2, 42, temp_str);
    
    // Temperature unit
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(95, 32, "°C");
    
    // Average humidity (large)
    u8g2.setFont(u8g2_font_ncenB24_tr);
    char hum_str[16];
    snprintf(hum_str, sizeof(hum_str), "%d", (int)data->humidity_avg);
    width = u8g2.getStrWidth(hum_str);
    u8g2.drawStr((128 - width) / 2, 64, hum_str);
    
    // Humidity unit
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(95, 54, "%");
    
    // Bottom row: H↑, H↓, Tank temp, Ventilator status
    u8g2.setFont(u8g2_font_ncenB08_tr);
    
    u8g2.drawStr(0, 64, "H↑");
    if (data->aht10_top_valid) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", (int)data->humidity_top);
        u8g2.drawStr(15, 64, buf);
    } else {
        u8g2.drawStr(15, 64, "ERR");
    }
    
    u8g2.drawStr(48, 64, "H↓");
    if (data->aht10_bottom_valid) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", (int)data->humidity_bottom);
        u8g2.drawStr(63, 64, buf);
    } else {
        u8g2.drawStr(63, 64, "ERR");
    }
    
    // Tank temperature
    if (data->ds18b20_valid) {
        char buf[20];
        snprintf(buf, sizeof(buf), "Б%.0f", data->temp_tank);
        u8g2.drawStr(95, 64, buf);
    }
    
    // Ventilator status
    if (ventilator_is_running()) {
        u8g2.drawStr(95, 10, "ВКЛ");
    } else {
        u8g2.drawStr(95, 10, "ВЫКЛ");
    }
    
    u8g2.sendBuffer();
}

// Draw temperature rate screen (Screen 1)
static void display_draw_temp_rate_screen() {
    float rate = sensors_get_temp_change_rate();
    
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    
    u8g2.drawStr(10, 15, "dT/dt");
    
    // Draw large number
    u8g2.setFont(u8g2_font_ncenB24_tr);
    char buf[20];
    snprintf(buf, sizeof(buf), "%.2f", fabs(rate));
    uint8_t width = u8g2.getStrWidth(buf);
    u8g2.drawStr((128 - width) / 2, 50, buf);
    
    // Draw arrow (↑, ↓, or =)
    u8g2.setFont(u8g2_font_ncenB36_tr);
    if (rate > 0.1f) {
        u8g2.drawStr(55, 45, "▲");
    } else if (rate < -0.1f) {
        u8g2.drawStr(55, 45, "▼");
    } else {
        u8g2.drawStr(58, 45, "=");
    }
    
    // Unit
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(95, 35, "°C/мин");
    
    u8g2.sendBuffer();
}

// Draw graph screen (Screen 2)
static void display_draw_graph_screen() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    
    if (graph_data.count == 0) {
        u8g2.drawStr(20, 30, "No data yet...");
        u8g2.sendBuffer();
        return;
    }
    
    // Draw temperature (solid line) and humidity (dashed line)
    // Graph area: x=10-120, y=10-55
    const uint8_t GRAPH_X_START = 10;
    const uint8_t GRAPH_X_END = 120;
    const uint8_t GRAPH_Y_START = 10;
    const uint8_t GRAPH_Y_END = 55;
    const uint8_t GRAPH_WIDTH = GRAPH_X_END - GRAPH_X_START;
    const uint8_t GRAPH_HEIGHT = GRAPH_Y_END - GRAPH_Y_START;
    
    // Draw axes
    u8g2.drawLine(GRAPH_X_START, GRAPH_Y_START, GRAPH_X_START, GRAPH_Y_END);
    u8g2.drawLine(GRAPH_X_START, GRAPH_Y_END, GRAPH_X_END, GRAPH_Y_END);
    
    // Find min/max for scaling
    float temp_min = 999, temp_max = -999;
    float hum_min = 999, hum_max = -999;
    
    for (uint8_t i = 0; i < graph_data.count; i++) {
        if (graph_data.temp[i] < temp_min) temp_min = graph_data.temp[i];
        if (graph_data.temp[i] > temp_max) temp_max = graph_data.temp[i];
        if (graph_data.humidity[i] < hum_min) hum_min = graph_data.humidity[i];
        if (graph_data.humidity[i] > hum_max) hum_max = graph_data.humidity[i];
    }
    
    float temp_range = (temp_max - temp_min > 0) ? (temp_max - temp_min) : 1.0f;
    float hum_range = (hum_max - hum_min > 0) ? (hum_max - hum_min) : 1.0f;
    
    // Draw lines
    for (uint8_t i = 0; i < graph_data.count - 1; i++) {
        uint8_t x1 = GRAPH_X_START + (i * GRAPH_WIDTH) / graph_data.count;
        uint8_t x2 = GRAPH_X_START + ((i + 1) * GRAPH_WIDTH) / graph_data.count;
        
        // Temperature line (solid)
        uint8_t y1 = GRAPH_Y_END - (uint8_t)((graph_data.temp[i] - temp_min) * GRAPH_HEIGHT / temp_range);
        uint8_t y2 = GRAPH_Y_END - (uint8_t)((graph_data.temp[i + 1] - temp_min) * GRAPH_HEIGHT / temp_range);
        u8g2.drawLine(x1, y1, x2, y2);
        
        // Humidity line (dashed)
        if (i % 2 == 0) {
            uint8_t y3 = GRAPH_Y_END - (uint8_t)((graph_data.humidity[i] - hum_min) * GRAPH_HEIGHT / hum_range);
            uint8_t y4 = GRAPH_Y_END - (uint8_t)((graph_data.humidity[i + 1] - hum_min) * GRAPH_HEIGHT / hum_range);
            u8g2.drawLine(x1, y3, x2, y4);
        }
    }
    
    // Legend
    u8g2.drawStr(10, 64, "T");
    u8g2.drawStr(25, 64, "H—");
    
    u8g2.sendBuffer();
}

// Draw error log screen (Screen 3)
static void display_draw_error_log_screen() {
    uint8_t error_count = errors_get_count();
    
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    
    if (error_count == 0) {
        u8g2.drawStr(20, 30, "No errors");
        u8g2.sendBuffer();
        return;
    }
    
    // Show last 3 errors
    error_entry_t* log = errors_get_log();
    uint8_t start_idx = (error_count > 3) ? (error_count - 3) : 0;
    
    uint8_t y = 15;
    for (uint8_t i = start_idx; i < error_count && i < start_idx + 3; i++) {
        char buf[40];
        snprintf(buf, sizeof(buf), "%d min %s", log[i].time_minutes, log[i].source);
        u8g2.drawStr(5, y, buf);
        y += 15;
    }
    
    char buf[20];
    snprintf(buf, sizeof(buf), "Errors: %d/10", error_count);
    u8g2.drawStr(10, 64, buf);
    
    u8g2.sendBuffer();
}

// Draw menu - ventilator time
static void display_draw_menu_ventilator_time() {
    settings_t* settings = settings_get();
    
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    
    u8g2.drawStr(10, 15, "Ventilator Time");
    
    u8g2.setFont(u8g2_font_ncenB24_tr);
    char buf[20];
    snprintf(buf, sizeof(buf), "%d", settings->ventilator_time);
    u8g2.drawStr(40, 50, buf);
    
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(75, 50, "min");
    
    u8g2.drawStr(10, 64, "Use START to change");
    
    u8g2.sendBuffer();
}

// Draw menu - humidity threshold
static void display_draw_menu_humidity() {
    settings_t* settings = settings_get();
    
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    
    u8g2.drawStr(10, 15, "Stop Humidity");
    
    u8g2.setFont(u8g2_font_ncenB24_tr);
    char buf[20];
    snprintf(buf, sizeof(buf), "%d", settings->stop_humidity);
    u8g2.drawStr(40, 50, buf);
    
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(75, 50, "%");
    
    u8g2.drawStr(10, 64, "Use START to change");
    
    u8g2.sendBuffer();
}

// Draw menu - Wi-Fi info
static void display_draw_menu_wifi_info() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    
    u8g2.drawStr(10, 15, "SSID: " WIFI_SSID);
    u8g2.drawStr(10, 27, WIFI_IP);
    
    char buf[20];
    snprintf(buf, sizeof(buf), "RSSI: %d dBm", wifi_get_rssi());
    u8g2.drawStr(10, 39, buf);
    
    snprintf(buf, sizeof(buf), "Clients: %d", wifi_get_clients_count());
    u8g2.drawStr(10, 51, buf);
    
    u8g2.sendBuffer();
}

// Update display based on current state
void display_update() {
    // Update graph every GRAPH_SAMPLE_INTERVAL seconds
    unsigned long now = millis();
    if (now - graph_data.last_sample_time > GRAPH_SAMPLE_INTERVAL * 1000) {
        graph_data.last_sample_time = now;
        
        sensors_data_t* data = sensors_get_data();
        if (data->aht10_top_valid || data->aht10_bottom_valid) {
            graph_data.temp[graph_data.index] = data->temp_avg;
            graph_data.humidity[graph_data.index] = data->humidity_avg;
            
            graph_data.index = (graph_data.index + 1) % GRAPH_MAX_POINTS;
            if (graph_data.count < GRAPH_MAX_POINTS) {
                graph_data.count++;
            }
        }
    }
    
    // Check menu timeout
    if (menu_mode && now - menu_timeout > MENU_TIMEOUT) {
        menu_mode = false;
        current_screen = 0;
    }
    
    // Draw appropriate screen
    if (menu_mode) {
        switch (menu_item) {
            case 0:
                display_draw_menu_ventilator_time();
                break;
            case 1:
                display_draw_menu_humidity();
                break;
            case 2:
                display_draw_menu_wifi_info();
                break;
            default:
                display_draw_main_screen();
                break;
        }
    } else {
        switch (current_screen) {
            case 0:
                display_draw_main_screen();
                break;
            case 1:
                display_draw_temp_rate_screen();
                break;
            case 2:
                display_draw_graph_screen();
                break;
            case 3:
                display_draw_error_log_screen();
                break;
            default:
                display_draw_main_screen();
                break;
        }
    }
}

// Show main screen
void display_show_main_screen() {
    current_screen = 0;
    menu_mode = false;
}

// Next screen
void display_next_screen() {
    if (!menu_mode) {
        current_screen = (current_screen + 1) % 4;
    } else {
        menu_item = (menu_item + 1) % 3;
    }
}

// Enter menu mode
void display_enter_menu() {
    menu_mode = true;
    menu_item = 0;
    menu_timeout = millis();
}

// Exit menu mode
void display_exit_menu() {
    menu_mode = false;
    current_screen = 0;
}

// Get menu state
bool display_is_in_menu() {
    return menu_mode;
}

// Get current menu item
uint8_t display_get_menu_item() {
    return menu_item;
}

// Get current screen
uint8_t display_get_current_screen() {
    return current_screen;
}
