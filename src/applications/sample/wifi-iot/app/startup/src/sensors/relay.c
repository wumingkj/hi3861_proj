/**
 ****************************************************************************************************
 * @file        relay.c
 * @author      普中科技
 * @version     V1.0
 * @date        2024-06-05
 * @brief       SRD-05VDC-SL-C继电器模块
 * @license     Copyright (c) 2024-2034, 深圳市普中科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:普中-Hi3861
 * 在线视频:https://space.bilibili.com/2146492485
 * 公司网址:www.prechin.cn
 * 购买地址:
 *
 */

#include "relay.h"
#include <unistd.h>
#include <stdio.h>

// 继电器状态变量
static relay_state_t g_relay_state = RELAY_OFF;

// 继电器初始化
void relay_init(void)
{
    // 配置GPIO为输出模式
    hi_io_set_func(RELAY_GPIO_PIN, RELAY_GPIO_FUNC);
    hi_gpio_set_dir(RELAY_GPIO_IDX, HI_GPIO_DIR_OUT);
    
    // 初始状态为断开
    relay_set_state(RELAY_OFF);
    
    printf("SRD-05VDC-SL-C继电器初始化完成（使用GPIO%d）\r\n", RELAY_GPIO_IDX);
}

// 设置继电器状态
void relay_set_state(relay_state_t state)
{
    g_relay_state = state;
    
    if (state == RELAY_ON) {
        // 继电器闭合（高电平触发）
        hi_gpio_set_ouput_val(RELAY_GPIO_IDX, HI_GPIO_VALUE1);
        printf("继电器状态：闭合（ON）- GPIO%d\r\n", RELAY_GPIO_IDX);
    } else {
        // 继电器断开（低电平）
        hi_gpio_set_ouput_val(RELAY_GPIO_IDX, HI_GPIO_VALUE0);
        printf("继电器状态：断开（OFF）- GPIO%d\r\n", RELAY_GPIO_IDX);
    }
}

// 获取继电器状态
relay_state_t relay_get_state(void)
{
    return g_relay_state;
}

// 切换继电器状态
void relay_toggle(void)
{
    if (g_relay_state == RELAY_ON) {
        relay_set_state(RELAY_OFF);
    } else {
        relay_set_state(RELAY_ON);
    }
}

// 继电器任务（用于周期性状态检测）
void relay_task(void)
{
    static uint32_t counter = 0;
    
    counter++;
    
    // 每5秒输出一次继电器状态
    if (counter % 500 == 0) {
        printf("继电器状态检测（GPIO%d）：%s\r\n", 
               RELAY_GPIO_IDX,
               (g_relay_state == RELAY_ON) ? "闭合（ON）" : "断开（OFF）");
    }
    
    Time_DelayMs(10);  // 10ms延时
}