#ifndef WIFI_H
#define WIFI_H

void wifi_init();
void wifi_update();
int8_t wifi_get_rssi();
uint8_t wifi_get_clients_count();
const char* wifi_get_ip();

#endif // WIFI_H
