#ifndef WIFI_H
#define WIFI_H

typedef enum {
    WIFI_CONNECT    = 0,
	WIFI_CONFIG,
} wifi_state_t;

void wifi_config(wifi_state_t state);
void wifi_uart_rx_callback(uint8_t rx_data);

#endif