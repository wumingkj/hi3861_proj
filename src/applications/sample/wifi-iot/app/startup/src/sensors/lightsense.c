/**
 ****************************************************************************************************
 * @file        lightsense.c
 * @author      普中科技
 * @version     V1.0
 * @date        2024-06-05
 * @brief       光敏传感器模块
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

#include "lightsense.h"
#include "adc.h"
#include <unistd.h>
#include <stdio.h>

// 光敏传感器阈值定义
#define LIGHTSENSE_DARK_THRESHOLD  500     // 黑暗阈值
#define LIGHTSENSE_LIGHT_THRESHOLD 1000    // 明亮阈值

// 光敏传感器初始化
void lightsense_init(void)
{
    adc4_init();  // 使用ADC4通道（GPIO6）
    printf("光敏传感器初始化完成（使用ADC4通道，GPIO6）\r\n");
}

// 获取光敏传感器原始值
uint16_t lightsense_get_value(void)
{
    return get_adc4_value();
}

// 获取光敏传感器电压值
float lightsense_get_voltage(void)
{
    return get_adc4_voltage();
}

// 获取光敏传感器状态
lightsense_state_t lightsense_get_state(void)
{
    uint16_t adc_value = lightsense_get_value();
    
    if (adc_value < LIGHTSENSE_DARK_THRESHOLD) {
        return LIGHTSENSE_DARK;
    } else if (adc_value > LIGHTSENSE_LIGHT_THRESHOLD) {
        return LIGHTSENSE_LIGHT;
    } else {
        // 中间状态，根据实际情况返回
        return (adc_value > (LIGHTSENSE_DARK_THRESHOLD + LIGHTSENSE_LIGHT_THRESHOLD) / 2) ? 
               LIGHTSENSE_LIGHT : LIGHTSENSE_DARK;
    }
}

// 光敏传感器任务
void lightsense_task(void)
{
    static uint32_t counter = 0;
    uint16_t adc_value;
    float voltage;
    lightsense_state_t state;
    
    counter++;
    
    if (counter % 50 == 0) {
        adc_value = lightsense_get_value();
        voltage = lightsense_get_voltage();
        state = lightsense_get_state();
        
        printf("光敏传感器（ADC4/GPIO6）- ADC值: %d, 电压: %.2fV, 状态: %s\r\n", 
               adc_value, voltage, 
               (state == LIGHTSENSE_DARK) ? "黑暗" : "明亮");
    }
    
    usleep(10 * 1000);  // 10ms延时
}