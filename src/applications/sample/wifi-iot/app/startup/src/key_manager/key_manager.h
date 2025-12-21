/**
 ****************************************************************************************************
 * @file        key_manager.h
 * @author      普中科技
 * @version     V1.0
 * @date        2024-12-19
 * @brief       高级按键管理器（支持长按、短按、双击检测）
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

#ifndef KEY_MANAGER_H
#define KEY_MANAGER_H

#include "cmsis_os2.h"
#include "hi_io.h"
#include "hi_gpio.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// 按键事件类型
typedef enum {
    KEY_EVENT_NONE = 0,        // 无事件
    KEY_EVENT_PRESS,           // 按下
    KEY_EVENT_RELEASE,         // 释放
    KEY_EVENT_SHORT_PRESS,     // 短按
    KEY_EVENT_LONG_PRESS,      // 长按
    KEY_EVENT_DOUBLE_CLICK,    // 双击
    KEY_EVENT_HOLD             // 保持按下
} key_event_t;

// 按键状态
typedef enum {
    KEY_STATE_RELEASED = 0,    // 释放状态
    KEY_STATE_PRESSED,         // 按下状态
    KEY_STATE_DEBOUNCING       // 消抖状态
} key_state_t;

// 按键配置结构体
typedef struct {
    hi_io_name pin;            // GPIO引脚
    uint8_t func;              // GPIO功能（使用uint8_t替代hi_io_func）
    uint32_t debounce_time;    // 消抖时间(ms)
    uint32_t long_press_time;  // 长按时间(ms)
    uint32_t double_click_time;// 双击间隔时间(ms)
} key_config_t;

// 按键信息结构体
typedef struct {
    key_config_t config;       // 按键配置
    key_state_t state;         // 当前状态
    key_state_t last_state;    // 上次状态
    uint32_t press_start_time; // 按下开始时间
    uint32_t last_release_time;// 上次释放时间
    uint8_t click_count;       // 点击计数
    bool is_double_click;      // 是否为双击
} key_info_t;

// 按键管理器类
typedef struct {
    key_info_t *keys;          // 按键数组指针（动态分配）
    uint8_t key_count;         // 按键数量
    uint8_t max_keys;          // 最大支持按键数
} key_manager_t;

// 函数声明

/**
 * @brief 创建按键管理器
 * @param max_keys 最大支持按键数
 * @return 按键管理器指针，失败返回NULL
 */
key_manager_t* key_manager_create(uint8_t max_keys);

/**
 * @brief 销毁按键管理器
 * @param manager 按键管理器指针
 */
void key_manager_destroy(key_manager_t *manager);

/**
 * @brief 添加按键到管理器
 * @param manager 按键管理器指针
 * @param pin GPIO引脚
 * @param func GPIO功能（可传0使用默认功能）
 * @param debounce_time 消抖时间(ms)，传0使用默认值
 * @param long_press_time 长按时间(ms)，传0使用默认值
 * @param double_click_time 双击间隔时间(ms)，传0使用默认值
 * @return 按键索引，失败返回-1
 */
int8_t key_manager_add_key(key_manager_t *manager, hi_io_name pin, uint8_t func,
                          uint32_t debounce_time, uint32_t long_press_time, uint32_t double_click_time);

/**
 * @brief 移除按键
 * @param manager 按键管理器指针
 * @param key_index 按键索引
 * @return 成功返回0，失败返回-1
 */
int8_t key_manager_remove_key(key_manager_t *manager, uint8_t key_index);

/**
 * @brief 更新按键状态（需要在主循环中定期调用）
 * @param manager 按键管理器指针
 */
void key_manager_update(key_manager_t *manager);

/**
 * @brief 获取按键事件
 * @param manager 按键管理器指针
 * @param key_index 按键索引
 * @return 按键事件类型
 */
key_event_t key_manager_get_event(key_manager_t *manager, uint8_t key_index);

/**
 * @brief 检查按键是否按下
 * @param manager 按键管理器指针
 * @param key_index 按键索引
 * @return true:按下, false:释放
 */
bool key_manager_is_pressed(key_manager_t *manager, uint8_t key_index);

/**
 * @brief 检查按键是否保持按下
 * @param manager 按键管理器指针
 * @param key_index 按键索引
 * @return true:保持按下, false:未保持
 */
bool key_manager_is_holding(key_manager_t *manager, uint8_t key_index);

/**
 * @brief 获取按键按下持续时间
 * @param manager 按键管理器指针
 * @param key_index 按键索引
 * @return 按下持续时间(ms)
 */
uint32_t key_manager_get_press_duration(key_manager_t *manager, uint8_t key_index);

/**
 * @brief 重置按键状态（清除所有事件）
 * @param manager 按键管理器指针
 * @param key_index 按键索引
 */
void key_manager_reset(key_manager_t *manager, uint8_t key_index);

/**
 * @brief 获取按键配置信息
 * @param manager 按键管理器指针
 * @param key_index 按键索引
 * @return 按键配置指针，失败返回NULL
 */
const key_config_t* key_manager_get_config(key_manager_t *manager, uint8_t key_index);

/**
 * @brief 修改按键配置
 * @param manager 按键管理器指针
 * @param key_index 按键索引
 * @param debounce_time 消抖时间(ms)
 * @param long_press_time 长按时间(ms)
 * @param double_click_time 双击间隔时间(ms)
 * @return 成功返回0，失败返回-1
 */
int8_t key_manager_set_config(key_manager_t *manager, uint8_t key_index,
                             uint32_t debounce_time, uint32_t long_press_time, uint32_t double_click_time);

#endif