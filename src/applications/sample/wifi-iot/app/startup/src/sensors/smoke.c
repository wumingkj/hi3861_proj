/**
 ****************************************************************************************************
 * @file        smoke.c
 * @author      普中科技
 * @version     V1.0
 * @date        2024-06-05
 * @brief       烟雾传感器模块库实现
 * @license     Copyright (c) 2024-2034, 深圳市普中科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:普中-Hi3861
 * 在线视频:https://space.bilibili.com/2146492485
 * 公司网址:www.prechin.cn
 * 购买地址:
 *
 ****************************************************************************************************
 * 功能描述：烟雾传感器模块库实现，支持MQ-2等烟雾传感器
 * 
 ****************************************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include "smoke.h"
#include "time.h"


// 烟雾浓度阈值配置（可动态调整）
static uint16_t smoke_thresholds[] = {
    SMOKE_THRESHOLD_LOW,
    SMOKE_THRESHOLD_MEDIUM, 
    SMOKE_THRESHOLD_HIGH,
    SMOKE_THRESHOLD_DANGER
};

// 烟雾浓度等级描述字符串
static const char* smoke_level_strings[] = {
    "无烟雾",
    "低浓度", 
    "中浓度",
    "高浓度",
    "危险浓度"
};

/**
 * @brief  初始化烟雾传感器
 */
void smoke_sensor_init(void)
{
    // 配置ADC5引脚功能
    hi_io_set_func(SMOKE_SENSOR_PIN, HI_IO_FUNC_GPIO_11_GPIO);
    
    // 初始化ADC通道（ADC5对应GPIO11）
    hi_adc_init(NULL, NULL);
    
    printf("[SMOKE] 烟雾传感器初始化完成\r\n");
    printf("[SMOKE] 使用引脚: GPIO11 (ADC5)\r\n");
}

/**
 * @brief  读取烟雾传感器原始ADC值
 */
uint16_t smoke_sensor_read_raw(void)
{
    hi_u16 data = 0;
    hi_adc_read(HI_ADC_CHANNEL_5, &data, HI_ADC_EQU_MODEL_4, HI_ADC_CUR_BAIS_DEFAULT, 0);
    return (uint16_t)data;
}

/**
 * @brief  读取烟雾传感器电压值
 */
float smoke_sensor_read_voltage(void)
{
    uint16_t raw_value = smoke_sensor_read_raw();
    // ADC参考电压3.3V，12位精度(0-4095)
    return (raw_value * 3.3f) / 4095.0f;
}

/**
 * @brief  获取烟雾浓度等级
 */
smoke_level_t smoke_sensor_get_level(void)
{
    uint16_t raw_value = smoke_sensor_read_raw();
    
    if (raw_value < smoke_thresholds[0]) {
        return SMOKE_LEVEL_NONE;
    } else if (raw_value < smoke_thresholds[1]) {
        return SMOKE_LEVEL_LOW;
    } else if (raw_value < smoke_thresholds[2]) {
        return SMOKE_LEVEL_MEDIUM;
    } else if (raw_value < smoke_thresholds[3]) {
        return SMOKE_LEVEL_HIGH;
    } else {
        return SMOKE_LEVEL_DANGER;
    }
}

/**
 * @brief  读取完整的烟雾传感器数据
 */
void smoke_sensor_read_data(smoke_sensor_data_t *data)
{
    if (data == NULL) return;
    
    data->raw_value = smoke_sensor_read_raw();
    data->voltage = smoke_sensor_read_voltage();
    data->level = smoke_sensor_get_level();
    data->timestamp = Time_GetCurrentMs();
    data->alarm_triggered = smoke_sensor_check_alarm(SMOKE_THRESHOLD_HIGH);
}

/**
 * @brief  检查是否触发烟雾报警
 */
bool smoke_sensor_check_alarm(uint16_t threshold)
{
    uint16_t raw_value = smoke_sensor_read_raw();
    return (raw_value >= threshold);
}

/**
 * @brief  设置烟雾报警阈值
 */
void smoke_sensor_set_threshold(smoke_level_t level, uint16_t threshold)
{
    if (level >= SMOKE_LEVEL_LOW && level <= SMOKE_LEVEL_DANGER) {
        smoke_thresholds[level - 1] = threshold;
        printf("[SMOKE] 设置 %s 阈值为: %d\r\n", 
               smoke_sensor_get_level_string(level), threshold);
    }
}

/**
 * @brief  获取烟雾浓度描述字符串
 */
const char* smoke_sensor_get_level_string(smoke_level_t level)
{
    if (level >= SMOKE_LEVEL_NONE && level <= SMOKE_LEVEL_DANGER) {
        return smoke_level_strings[level];
    }
    return "未知等级";
}

/**
 * @brief  烟雾传感器测试函数
 */
void smoke_sensor_test(void)
{
    printf("\r\n=== 烟雾传感器测试开始 ===\r\n");
    
    smoke_sensor_init();
    
    for (int i = 0; i < 10; i++) {
        smoke_sensor_data_t data;
        smoke_sensor_read_data(&data);
        
        printf("[SMOKE] 测试 %d: ADC值=%d, 电压=%.2fV, 等级=%s", 
               i + 1, data.raw_value, data.voltage, 
               smoke_sensor_get_level_string(data.level));
        
        if (data.alarm_triggered) {
            printf(" [报警!]");
        }
        printf("\r\n");
        
        Time_DelayMs(500);
    }
    
    printf("=== 烟雾传感器测试结束 ===\r\n");
}

// 烟雾传感器任务函数（可选，用于持续监控）
void smoke_sensor_task(void *arg)
{
    (void)arg;
    
    smoke_sensor_init();
    printf("[SMOKE] 烟雾传感器监控任务启动\r\n");
    
    uint32_t last_display_time = 0;
    const uint32_t display_interval = 2000; // 2秒显示一次
    
    while (1) {
        uint32_t current_time = Time_GetCurrentMs();
        
        // 定期显示传感器数据
        if (current_time - last_display_time >= display_interval) {
            last_display_time = current_time;
            
            smoke_sensor_data_t data;
            smoke_sensor_read_data(&data);
            
            printf("[SMOKE] 状态: ADC=%d, 电压=%.2fV, 等级=%s", 
                   data.raw_value, data.voltage, 
                   smoke_sensor_get_level_string(data.level));
            
            if (data.alarm_triggered) {
                printf(" ⚠️ 烟雾报警!");
            }
            printf("\r\n");
        }
        
        // 检查报警状态（实时监控）
        if (smoke_sensor_check_alarm(SMOKE_THRESHOLD_HIGH)) {
            printf("[SMOKE] 🚨 烟雾浓度过高！请检查环境安全！\r\n");
        }
        
        Time_DelayMs(100);
    }
}