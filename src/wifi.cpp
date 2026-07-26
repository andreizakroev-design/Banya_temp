#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "config.h"
#include "wifi.h"
#include "sensors.h"
#include "ventilator.h"
#include "settings.h"

// Web server
AsyncWebServer server(80);

// Wi-Fi state
static int8_t wifi_rssi = -100;
static uint8_t wifi_clients_count = 0;

// Initialize Wi-Fi
void wifi_init() {
    // Set Wi-Fi mode to Access Point
    WiFi.mode(WIFI_AP);
    
    // Configure AP
    IPAddress local_ip(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    
    // Start AP (no password)
    WiFi.softAP(WIFI_SSID);
    
    Serial.print("[WiFi] Access Point started: ");
    Serial.println(WIFI_SSID);
    Serial.print("[WiFi] IP: ");
    Serial.println(WiFi.softAPIP());
    
    // Setup web server routes
    setup_web_routes();
    
    // Start server
    server.begin();
    Serial.println("[WiFi] Web server started on port 80");
}

// Setup web server routes
void setup_web_routes() {
    // Main page (HTML)
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Banya Control</title>
    <style>
        body { font-family: Arial; margin: 10px; background: #f0f0f0; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        h1 { color: #333; text-align: center; }
        .section { margin: 20px 0; padding: 15px; background: #f9f9f9; border-left: 4px solid #007bff; border-radius: 4px; }
        .parameter { display: flex; justify-content: space-between; margin: 10px 0; }
        .label { font-weight: bold; }
        .value { color: #007bff; font-size: 1.1em; }
        .control { display: flex; gap: 10px; margin: 15px 0; }
        button { padding: 10px 20px; font-size: 1em; border: none; border-radius: 4px; cursor: pointer; }
        .btn-on { background: #28a745; color: white; }
        .btn-on:hover { background: #218838; }
        .btn-off { background: #dc3545; color: white; }
        .btn-off:hover { background: #c82333; }
        .btn-adjust { background: #ffc107; color: black; }
        .btn-adjust:hover { background: #e0a800; }
        input { padding: 8px; font-size: 1em; border: 1px solid #ddd; border-radius: 4px; width: 60px; }
        .ventilator-on { color: #28a745; font-weight: bold; }
        .ventilator-off { color: #dc3545; font-weight: bold; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌡️ Banya Control</h1>
        
        <div class="section">
            <h2>Температура & Влажность</h2>
            <div class="parameter">
                <span class="label">Сверху:</span>
                <span class="value"><span id="t_top">--</span>°C / <span id="h_top">--</span>%</span>
            </div>
            <div class="parameter">
                <span class="label">Снизу:</span>
                <span class="value"><span id="t_bottom">--</span>°C / <span id="h_bottom">--</span>%</span>
            </div>
            <div class="parameter">
                <span class="label">Среднее:</span>
                <span class="value"><span id="t_avg">--</span>°C / <span id="h_avg">--</span>%</span>
            </div>
            <div class="parameter">
                <span class="label">Бак:</span>
                <span class="value"><span id="t_tank">--</span>°C</span>
            </div>
        </div>
        
        <div class="section">
            <h2>Вентилятор</h2>
            <div class="parameter">
                <span class="label">Статус:</span>
                <span id="vent_status" class="value ventilator-off">ВЫКЛЮЧЕН</span>
            </div>
            <div class="parameter">
                <span class="label">Осталось:</span>
                <span class="value"><span id="vent_time">0</span> сек</span>
            </div>
            <div class="control">
                <button class="btn-on" onclick="ventStart()">Включить</button>
                <button class="btn-off" onclick="ventStop()">Выключить</button>
            </div>
        </div>
        
        <div class="section">
            <h2>Настройки</h2>
            <div class="parameter">
                <span class="label">Время проветривания:</span>
                <span>
                    <input type="number" id="vent_time_input" min="5" max="120" step="5" value="30">
                    <button class="btn-adjust" onclick="setVentTime()">Установить</button>
                    мин
                </span>
            </div>
            <div class="parameter">
                <span class="label">Влажность остановки:</span>
                <span>
                    <input type="number" id="hum_threshold_input" min="10" max="90" step="5" value="30">
                    <button class="btn-adjust" onclick="setHumThreshold()">Установить</button>
                    %
                </span>
            </div>
        </div>
        
        <div class="section">
            <h2>Wi-Fi</h2>
            <div class="parameter">
                <span class="label">Сигнал:</span>
                <span class="value"><span id="rssi">--</span> dBm</span>
            </div>
            <div class="parameter">
                <span class="label">Клиентов:</span>
                <span class="value"><span id="clients">0</span></span>
            </div>
        </div>
    </div>
    
    <script>
        function updateData() {
            fetch('/api/data')
                .then(r => r.json())
                .then(d => {
                    document.getElementById('t_top').textContent = d.t_top.toFixed(1);
                    document.getElementById('h_top').textContent = Math.round(d.h_top);
                    document.getElementById('t_bottom').textContent = d.t_bottom.toFixed(1);
                    document.getElementById('h_bottom').textContent = Math.round(d.h_bottom);
                    document.getElementById('t_avg').textContent = d.t_avg.toFixed(1);
                    document.getElementById('h_avg').textContent = Math.round(d.h_avg);
                    document.getElementById('t_tank').textContent = d.t_tank.toFixed(1);
                    document.getElementById('vent_status').textContent = d.vent_running ? 'ВКЛЮЧЕН' : 'ВЫКЛЮЧЕН';
                    document.getElementById('vent_status').className = d.vent_running ? 'value ventilator-on' : 'value ventilator-off';
                    document.getElementById('vent_time').textContent = d.vent_remaining;
                    document.getElementById('vent_time_input').value = d.vent_time;
                    document.getElementById('hum_threshold_input').value = d.hum_threshold;
                    document.getElementById('rssi').textContent = d.rssi;
                    document.getElementById('clients').textContent = d.clients;
                })
                .catch(e => console.log('Error:', e));
        }
        
        function ventStart() {
            fetch('/api/vent/start', {method: 'POST'});
        }
        
        function ventStop() {
            fetch('/api/vent/stop', {method: 'POST'});
        }
        
        function setVentTime() {
            let time = document.getElementById('vent_time_input').value;
            fetch('/api/settings/vent_time', {
                method: 'POST',
                body: JSON.stringify({value: parseInt(time)})
            });
        }
        
        function setHumThreshold() {
            let hum = document.getElementById('hum_threshold_input').value;
            fetch('/api/settings/hum_threshold', {
                method: 'POST',
                body: JSON.stringify({value: parseInt(hum)})
            });
        }
        
        // Update every 1 second
        setInterval(updateData, 1000);
        updateData(); // Initial update
    </script>
</body>
</html>
        )";
        request->send(200, "text/html", html);
    });
    
    // API: Get current data
    server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *request) {
        sensors_data_t* data = sensors_get_data();
        settings_t* settings = settings_get();
        
        String json = "{";
        json += "\"t_top\":" + String(data->temp_top, 1) + ",";
        json += "\"h_top\":" + String(data->humidity_top, 1) + ",";
        json += "\"t_bottom\":" + String(data->temp_bottom, 1) + ",";
        json += "\"h_bottom\":" + String(data->humidity_bottom, 1) + ",";
        json += "\"t_avg\":" + String(data->temp_avg, 1) + ",";
        json += "\"h_avg\":" + String(data->humidity_avg, 1) + ",";
        json += "\"t_tank\":" + String(data->temp_tank, 1) + ",";
        json += "\"vent_running\":" + String(ventilator_is_running() ? "true" : "false") + ",";
        json += "\"vent_remaining\":" + String(ventilator_get_remaining_time()) + ",";
        json += "\"vent_time\":" + String(settings->ventilator_time) + ",";
        json += "\"hum_threshold\":" + String(settings->stop_humidity) + ",";
        json += "\"rssi\":" + String(WiFi.softAPgetStationNum() > 0 ? -50 : -100) + ",";
        json += "\"clients\":" + String(WiFi.softAPgetStationNum());
        json += "}";
        
        request->send(200, "application/json", json);
    });
    
    // API: Start ventilator
    server.on("/api/vent/start", HTTP_POST, [](AsyncWebServerRequest *request) {
        ventilator_start();
        request->send(200);
    });
    
    // API: Stop ventilator
    server.on("/api/vent/stop", HTTP_POST, [](AsyncWebServerRequest *request) {
        ventilator_stop();
        request->send(200);
    });
    
    // API: Set ventilator time
    server.on("/api/settings/vent_time", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("body", true)) {
            // Parse JSON (simplified)
            String body = request->getParam("body", true)->value();
            // Extract value from JSON
            int idx = body.indexOf("value");
            if (idx >= 0) {
                int value = body.substring(idx + 7).toInt();
                settings_set_ventilator_time(value);
                ventilator_set_time(value);
            }
        }
        request->send(200);
    });
    
    // API: Set humidity threshold
    server.on("/api/settings/hum_threshold", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("body", true)) {
            String body = request->getParam("body", true)->value();
            int idx = body.indexOf("value");
            if (idx >= 0) {
                int value = body.substring(idx + 7).toInt();
                settings_set_stop_humidity(value);
                ventilator_set_stop_humidity(value);
            }
        }
        request->send(200);
    });
}

// Update Wi-Fi state
void wifi_update() {
    wifi_clients_count = WiFi.softAPgetStationNum();
}

// Get RSSI
int8_t wifi_get_rssi() {
    return wifi_rssi;
}

// Get clients count
uint8_t wifi_get_clients_count() {
    return wifi_clients_count;
}

// Get IP address
const char* wifi_get_ip() {
    static char ip_str[16];
    IPAddress ip = WiFi.softAPIP();
    snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return ip_str;
}
