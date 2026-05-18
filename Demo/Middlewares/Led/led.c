#include "../global.h"
#include "led.h"
#include "../Event/event.h"
#include "../Log/log.h"
#include <stdint.h>

#define LED_PORT GPIOC
#define LED_PIN  GPIO_PIN_0

// PWM 呼吸灯数值数组 (256步，gamma=2.2校正，符合视觉感知)
const uint8_t pwm_breath[] = {
    185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200,
    201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216,
    217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232,
    233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248,
    249, 250, 251, 252, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

// 全局事件管理器
extern event_manager_t g_event_mgr;

/**
 * @brief LED控制函数：开灯
 * 
 * @return void
 */
static void led_on(void)
{
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
}

/**
 *  @brief LED控制函数：关灯
 * 
 * @return void
 */
static void led_off(void)
{
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

/**
 * @brief LED控制函数：切换灯状态
 * 
 * @return void
 */
static void led_toggle(void)
{
    HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
}

/**
 * @brief LED事件处理函数
 * 
 * @param event 事件类型
 * @param data 事件数据
 * @return int 返回值
 */
static int led_blink_handler(int event, void *data)
{
    switch (event)
    {
    case EVENT_SYS_STARTUP:
        led_on();
        break;  
    case EVENT_CHARGE_START:
        /* code */
        break;
    case EVENT_CHARGE_STOP:
        /* code */
        break;
    case EVENT_SLEEP_MODE:
        /* code */
        break;
    default:
        break;
    }

    // 这里添加LED控制逻辑
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
static int led_init(void)
{
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
static void led_task(void *pvParameters)
{
    // 注册到事件管理器，监听 LED相关事件
    notifier_register_event(&g_event_mgr, &led_node);
    // char temp3[256] = {0};
    // 这里可以添加LED的定时任务逻辑，例如呼吸灯效果
    // 可以使用pwm_breath数组来设置PWM占空比，实现平滑的呼吸灯效果
    while(1)
    {
        // HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        // LOG_I(MODULE_SHELL, "LED Toggle\r\n");
        vTaskDelay(500);
    }
}
MODULE_TASK(led_task, "LED_Task", 256, 1);