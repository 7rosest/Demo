#include "../global.h"
#include "led.h"
#include "../Event/event.h"
#include "../Log/log.h"
#include <stdint.h>

#define LED_PORT GPIOC
#define LED_PIN  GPIO_PIN_0

uint8_t led_state = 0;

/**
 * @brief LED控制函数：开灯
 * 
 * @return void
 */
static void led_on(void) {
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
}

/**
 *  @brief LED控制函数：关灯
 * 
 * @return void
 */
static void led_off(void) {
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

/**
 * @brief LED控制函数：切换灯状态
 * 
 * @return void
 */
static void led_toggle(void) {
    HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
}

/**
 * @brief LED事件处理函数
 * 
 * @param event 事件类型
 * @param data 事件数据
 * @return int 返回值
 */
static int led_blink_handler(int event, void *data) {
    switch (event) {
        case EVENT_SYS_STARTUP:
            led_on();
            break;  
        case EVENT_CHARGE_START:
            led_state = 1;
            break;
        case EVENT_CHARGE_STOP:
            /* code */
            break;
        case EVENT_SLEEP_MODE:
            led_off();
            /* code */
            break;
        default:
            break;
    }

    return 0;  // 继续传播
}

// LED notifier node
static notifier_node_t led_node = {
    .handler = led_blink_handler,
    .priority = 1,  // 优先级，数值越大优先级越高
};

/**
 * @brief 初始化LED模块
 * 
 * @return int 返回值
 */
static int led_init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);

    /*Configure GPIO pins : GPUO_Pin_0 */
    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    return 0;
}
MODULE_INIT(led_init, "LED");

/**
 * @brief LED任务函数
 * 
 * @param pvParameters 参数
 */
static void led_task(void *pvParameters) {
    #include "../Event/event.h"
    notifier_register_event(&g_event_mgr, &led_node);

    while(1) {
        if (led_state == 1) {
            led_toggle();
        }

        vTaskDelay(500);
    }
}
MODULE_TASK(led_task, "LED_Task", 256, 1);
