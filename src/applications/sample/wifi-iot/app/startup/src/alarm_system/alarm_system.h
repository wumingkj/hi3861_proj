#ifndef ALARM_SYSTEM_H
#define ALARM_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>  // 添加printf头文件

// 报警级别定义
typedef enum {
    ALARM_LEVEL_NONE = 0,     // 无报警
    ALARM_LEVEL_YELLOW = 1,   // 黄色报警
    ALARM_LEVEL_RED = 2       // 红色报警
} alarm_level_t;

// 报警类型定义
typedef enum {
    ALARM_TYPE_TEMPERATURE = 0,  // 温度报警
    ALARM_TYPE_HUMIDITY = 1,     // 湿度报警
    ALARM_TYPE_SMOKE = 2         // 烟雾报警
} alarm_type_t;

// 报警事件结构体
typedef struct {
    alarm_type_t type;        // 报警类型
    alarm_level_t level;      // 报警级别
    uint32_t value;            // 当前值（改为整型）
    uint32_t threshold;        // 阈值（改为整型）
    uint32_t timestamp;       // 时间戳
} alarm_event_t;

// 报警回调函数类型定义
typedef void (*alarm_callback_t)(const alarm_event_t* event);

// 报警系统配置结构体
typedef struct {
    // 温度阈值配置（改为整型，单位：0.1°C）
    uint32_t temp_yellow_threshold;  // 黄色报警温度阈值
    uint32_t temp_red_threshold;     // 红色报警温度阈值
    
    // 湿度阈值配置（改为整型，单位：0.1%）
    uint32_t hum_yellow_threshold;   // 黄色报警湿度阈值
    uint32_t hum_red_threshold;      // 红色报警湿度阈值
    
    // 烟雾阈值配置（保持整型）
    uint32_t smoke_yellow_threshold; // 黄色报警烟雾阈值
    uint32_t smoke_red_threshold;    // 红色报警烟雾阈值
} alarm_config_t;

// 报警系统状态结构体
typedef struct {
    alarm_level_t temp_alarm_level;   // 温度报警级别
    alarm_level_t hum_alarm_level;    // 湿度报警级别
    alarm_level_t smoke_alarm_level;  // 烟雾报警级别
    uint32_t last_update_time;        // 最后更新时间
} alarm_status_t;

/**
 * @brief 初始化报警系统
 * @param config 报警系统配置
 * @return true 成功，false 失败
 */
bool alarm_system_init(const alarm_config_t* config);

/**
 * @brief 释放报警系统资源
 */
void alarm_system_deinit(void);

/**
 * @brief 更新传感器数据并检测报警（改为整型）
 * @param temperature 温度值（单位：0.1°C）
 * @param humidity 湿度值（单位：0.1%）
 * @param smoke 烟雾浓度值
 * @return true 有报警，false 无报警
 */
bool alarm_system_update(uint32_t temperature, uint32_t humidity, uint32_t smoke);

/**
 * @brief 设置报警回调函数
 * @param callback 回调函数指针
 */
void alarm_system_set_callback(alarm_callback_t callback);

/**
 * @brief 获取报警系统状态
 * @param status 状态结构体指针
 */
void alarm_system_get_status(alarm_status_t* status);

/**
 * @brief 手动触发报警
 * @param type 报警类型
 * @param level 报警级别
 * @return true 成功，false 失败
 */
bool alarm_system_trigger_alarm(alarm_type_t type, alarm_level_t level);

/**
 * @brief 清除报警
 * @param type 报警类型
 * @return true 成功，false 失败
 */
bool alarm_system_clear_alarm(alarm_type_t type);

/**
 * @brief 获取报警级别字符串
 * @param level 报警级别
 * @return 字符串描述
 */
const char* alarm_system_get_level_string(alarm_level_t level);

/**
 * @brief 获取报警类型字符串
 * @param type 报警类型
 * @return 字符串描述
 */
const char* alarm_system_get_type_string(alarm_type_t type);

/**
 * @brief 获取当前报警配置
 * @param config 配置结构体指针
 */
void alarm_system_get_config(alarm_config_t* config);

/**
 * @brief 从KV存储加载报警配置
 * @param config 配置结构体指针
 * @return true 成功加载，false 加载失败（使用默认值）
 */
static bool alarm_system_load_config_from_kv(alarm_config_t* config);


#endif // ALARM_SYSTEM_H