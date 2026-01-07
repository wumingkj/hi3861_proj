/**
 ****************************************************************************************************
 * @file        smoke.h
 * @author      普中科技
 * @version     V1.0
 * @date        2024-06-05
 * @brief       烟雾传感器模块库
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
 * 功能描述：烟雾传感器模块库，支持MQ-2等烟雾传感器
 * 
 ****************************************************************************************************
 */

#ifndef SMOKE_H
#define SMOKE_H

#include "cmsis_os2.h"
#include "hi_io.h"
#include "hi_gpio.h"
#include "hi_adc.h"

#include "pin_definitions.h"


// 烟雾浓度阈值定义（可根据实际传感器调整）
#define SMOKE_THRESHOLD_LOW      500     // 低浓度阈值
#define SMOKE_THRESHOLD_MEDIUM   1000    // 中浓度阈值
#define SMOKE_THRESHOLD_HIGH     1500    // 高浓度阈值
#define SMOKE_THRESHOLD_DANGER   2000    // 危险浓度阈值

// 烟雾浓度等级枚举
typedef enum {
    SMOKE_LEVEL_NONE = 0,        // 无烟雾
    SMOKE_LEVEL_LOW,             // 低浓度
    SMOKE_LEVEL_MEDIUM,          // 中浓度
    SMOKE_LEVEL_HIGH,            // 高浓度
    SMOKE_LEVEL_DANGER           // 危险浓度
} smoke_level_t;

// 烟雾传感器状态结构体
typedef struct {
    uint16_t raw_value;          // 原始ADC值
    float voltage;               // 电压值
    smoke_level_t level;          // 烟雾浓度等级
    uint32_t timestamp;          // 时间戳
    bool alarm_triggered;        // 报警是否触发
} smoke_sensor_data_t;

// 函数声明

/**
 * @brief  初始化烟雾传感器
 * @note   初始化ADC通道和GPIO配置
 * @retval 无
 */
void smoke_sensor_init(void);

/**
 * @brief  读取烟雾传感器原始ADC值
 * @note   读取ADC5通道的原始数值
 * @retval 原始ADC值 (0-4095)
 */
uint16_t smoke_sensor_read_raw(void);

/**
 * @brief  读取烟雾传感器电压值
 * @note   将ADC值转换为电压值 (0-3.3V)
 * @retval 电压值 (单位: V)
 */
float smoke_sensor_read_voltage(void);

/**
 * @brief  获取烟雾浓度等级
 * @note   根据ADC值判断烟雾浓度等级
 * @retval 烟雾浓度等级
 */
smoke_level_t smoke_sensor_get_level(void);

/**
 * @brief  读取完整的烟雾传感器数据
 * @note   获取包含原始值、电压、等级等完整数据
 * @param  data: 存储传感器数据的结构体指针
 * @retval 无
 */
void smoke_sensor_read_data(smoke_sensor_data_t *data);

/**
 * @brief  检查是否触发烟雾报警
 * @note   根据设定的阈值检查是否触发报警
 * @param  threshold: 报警阈值 (默认使用SMOKE_THRESHOLD_HIGH)
 * @retval true: 触发报警, false: 未触发报警
 */
bool smoke_sensor_check_alarm(uint16_t threshold);

/**
 * @brief  设置烟雾报警阈值
 * @note   设置不同浓度等级的阈值
 * @param  level: 浓度等级
 * @param  threshold: 对应的阈值
 * @retval 无
 */
void smoke_sensor_set_threshold(smoke_level_t level, uint16_t threshold);

/**
 * @brief  获取烟雾浓度描述字符串
 * @note   根据浓度等级返回对应的描述字符串
 * @param  level: 浓度等级
 * @retval 描述字符串
 */
const char* smoke_sensor_get_level_string(smoke_level_t level);

/**
 * @brief  烟雾传感器测试函数
 * @note   用于测试烟雾传感器功能
 * @retval 无
 */
void smoke_sensor_test(void);

#endif /* SMOKE_H */