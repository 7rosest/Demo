#include "../global.h"
#include "usart.h"
#include "../Wifi/wifi.h"
#include "string.h"
#include "cmsis_os.h"

static int wifi_rx_cnt = 0;
uint8_t wifi_data[1024] = {0};

extern osMessageQId wifiQueueHandle;

/**
 * @brief 配置wifi模块,外部调用
 * 
 * @param state wifi状态
 */
void wifi_config(wifi_state_t state) {
	switch (state) {
		case WIFI_CONNECT: {
			/*配置wifi连接*/
            char *wifi_cmd = "AT+CWMODE=1\r\n";
			wifi_uart_send((uint8_t *)wifi_cmd, strlen(wifi_cmd));  

			/*配置wifi连接*/
            wifi_cmd = "AT+CWJAP=\"ChinaNet-qw7u\",\"sv95ksue\"\r\n";
			wifi_uart_send((uint8_t *)wifi_cmd, strlen(wifi_cmd));
			break;
		}
		case WIFI_CONFIG: {
			/*配置MQTT参数*/
            char *wifi_cmd = "AT+MQTTUSERCFG=0,1,\"i4ylXpMGPgg.STM32|securemode=2\\,signmethod=hmacsha256\\,timestamp=1731746784252|\",\"STM32&i4ylXpMGPgg\",\"a5cbec53c8f5efe1ef709a2aa32ebceb7d09ae19744f875bbeb1ae1cc2dfb7e8\",0,0,\"\"\r\n";
			wifi_uart_send((uint8_t *)wifi_cmd, strlen(wifi_cmd));

			wifi_cmd = "AT+MQTTCONN=0,\"iot-06z00an14dn7rjm.mqtt.iothub.aliyuncs.com\",1883,1\r\n";
			wifi_uart_send((uint8_t *)wifi_cmd, strlen(wifi_cmd));
			break;
		}
		default:
			break;
	}
}

/**
 * @brief WIFI UART接收回调函数
 * @param rx_data 接收到的字节数据
 * 
 * @return void
 */
void wifi_uart_rx_callback(uint8_t rx_data) {
    uint8_t temp_char = rx_data;
    static uint8_t last_char;

    if (wifi_rx_cnt >= 1024) {
        wifi_rx_cnt = 0;
    }
    wifi_data[wifi_rx_cnt++] = temp_char;

    if (last_char == '\r' && temp_char == '\n') {
        wifi_data[wifi_rx_cnt] = '\0';
        wifi_rx_cnt = 0;
        xQueueSendFromISR(wifiQueueHandle, wifi_data, NULL);
    }
    last_char = temp_char;
}

/**
 * @brief 初始化wifi模块
 * 
 * @return int
 */
static int wifi_init(void) {
    char *wifi_cmd = "AT+CWMODE=1\r\n";
    wifi_uart_send((uint8_t *)wifi_cmd, strlen(wifi_cmd));

    wifi_cmd = "AT+CWJAP=\"ChinaNet-qw7u\",\"sv95ksue\"\r\n";
    wifi_uart_send((uint8_t *)wifi_cmd, strlen(wifi_cmd));

    return 0;
}
MODULE_INIT(wifi_init, "WIFI");