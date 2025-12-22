/**
 ****************************************************************************************************
 * @file        key_manager.c
 * @author      普中科技
 * @version     V1.0
 * @date        2024-12-19
 * @brief       高级按键管理器实现
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

#include "key_manager.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "time.h"

// 获取当前时间（毫秒）
static uint32_t get_current_time(void) {
    return Time_GetCurrentMs();
}

// 读取GPIO输入值
static bool read_gpio_input(hi_io_name pin) {
    hi_gpio_value value;
    hi_gpio_get_input_val((hi_gpio_idx)pin, &value);
    return (value == HI_GPIO_VALUE0); // 低电平表示按下（上拉模式）
}

// 获取GPIO对应的默认功能
static uint8_t get_default_gpio_func(hi_io_name pin) {
    switch (pin) {
        case HI_IO_NAME_GPIO_0: return 0; // HI_IO_FUNC_GPIO_0_GPIO
        case HI_IO_NAME_GPIO_1: return 0; // HI_IO_FUNC_GPIO_1_GPIO
        case HI_IO_NAME_GPIO_2: return 0; // HI_IO_FUNC_GPIO_2_GPIO
        case HI_IO_NAME_GPIO_3: return 0; // HI_IO_FUNC_GPIO_3_GPIO
        case HI_IO_NAME_GPIO_4: return 0; // HI_IO_FUNC_GPIO_4_GPIO
        case HI_IO_NAME_GPIO_5: return 0; // HI_IO_FUNC_GPIO_5_GPIO
        case HI_IO_NAME_GPIO_6: return 0; // HI_IO_FUNC_GPIO_6_GPIO
        case HI_IO_NAME_GPIO_7: return 0; // HI_IO_FUNC_GPIO_7_GPIO
        case HI_IO_NAME_GPIO_8: return 0; // HI_IO_FUNC_GPIO_8_GPIO
        case HI_IO_NAME_GPIO_9: return 0; // HI_IO_FUNC_GPIO_9_GPIO
        case HI_IO_NAME_GPIO_10: return 0; // HI_IO_FUNC_GPIO_10_GPIO
        case HI_IO_NAME_GPIO_11: return 0; // HI_IO_FUNC_GPIO_11_GPIO
        case HI_IO_NAME_GPIO_12: return 0; // HI_IO_FUNC_GPIO_12_GPIO
        case HI_IO_NAME_GPIO_13: return 0; // HI_IO_FUNC_GPIO_13_GPIO
        case HI_IO_NAME_GPIO_14: return 0; // HI_IO_FUNC_GPIO_14_GPIO
        default: return 0;
    }
}

// 初始化单个按键
static void init_single_key(key_info_t *key, hi_io_name pin, uint8_t func,
                           uint32_t debounce_time, uint32_t long_press_time, uint32_t double_click_time) {
    // 配置参数
    key->config.pin = pin;
    key->config.func = (func == 0) ? get_default_gpio_func(pin) : func;
    key->config.debounce_time = (debounce_time == 0) ? 10 : debounce_time;     // 默认10ms消抖
    key->config.long_press_time = (long_press_time == 0) ? 1000 : long_press_time; // 默认1秒长按
    key->config.double_click_time = (double_click_time == 0) ? 300 : double_click_time; // 默认300ms双击间隔
    
    // 初始化状态
    key->state = KEY_STATE_RELEASED;
    key->last_state = KEY_STATE_RELEASED;
    key->press_start_time = 0;
    key->last_release_time = 0;
    key->click_count = 0;
    key->is_double_click = false;
    key->long_press_triggered = false;  // 新增：初始化长按触发标志
    
    // 配置GPIO
    hi_io_set_pull(pin, HI_IO_PULL_UP);
    hi_io_set_func(pin, key->config.func);
    hi_gpio_set_dir((hi_gpio_idx)pin, HI_GPIO_DIR_IN);
}

key_manager_t* key_manager_create(uint8_t max_keys) {
    if (max_keys == 0) return NULL;
    
    // 分配管理器内存
    key_manager_t *manager = (key_manager_t*)malloc(sizeof(key_manager_t));
    if (manager == NULL) return NULL;
    
    // 分配按键数组内存
    manager->keys = (key_info_t*)malloc(sizeof(key_info_t) * max_keys);
    if (manager->keys == NULL) {
        free(manager);
        return NULL;
    }
    
    // 初始化管理器
    manager->key_count = 0;
    manager->max_keys = max_keys;
    memset(manager->keys, 0, sizeof(key_info_t) * max_keys);
    
    // 初始化GPIO子系统
    hi_gpio_init();
    
    return manager;
}

void key_manager_destroy(key_manager_t *manager) {
    if (manager == NULL) return;
    
    if (manager->keys != NULL) {
        free(manager->keys);
    }
    free(manager);
}

int8_t key_manager_add_key(key_manager_t *manager, hi_io_name pin, uint8_t func,
                          uint32_t debounce_time, uint32_t long_press_time, uint32_t double_click_time) {
    if (manager == NULL || manager->key_count >= manager->max_keys) {
        return -1;
    }
    
    // 检查引脚是否已存在
    for (uint8_t i = 0; i < manager->key_count; i++) {
        if (manager->keys[i].config.pin == pin) {
            return -1; // 引脚已存在
        }
    }
    
    // 初始化新按键
    init_single_key(&manager->keys[manager->key_count], pin, func, 
                   debounce_time, long_press_time, double_click_time);
    
    manager->key_count++;
    return manager->key_count - 1;
}

int8_t key_manager_remove_key(key_manager_t *manager, uint8_t key_index) {
    if (manager == NULL || key_index >= manager->key_count) {
        return -1;
    }
    
    // 将后面的按键前移
    for (uint8_t i = key_index; i < manager->key_count - 1; i++) {
        manager->keys[i] = manager->keys[i + 1];
    }
    
    // 清空最后一个按键
    memset(&manager->keys[manager->key_count - 1], 0, sizeof(key_info_t));
    manager->key_count--;
    
    return 0;
}

void key_manager_update(key_manager_t *manager) {
    if (manager == NULL) return;
    
    uint32_t current_time = get_current_time();
    
    for (uint8_t i = 0; i < manager->key_count; i++) {
        key_info_t *key = &manager->keys[i];
        bool current_pressed = read_gpio_input(key->config.pin);
        
        // 保存上次状态
        key->last_state = key->state;
        
        // 状态机处理
        switch (key->state) {
            case KEY_STATE_RELEASED:
                if (current_pressed) {
                    // 检测到按下，进入消抖状态
                    key->state = KEY_STATE_DEBOUNCING;
                    key->press_start_time = current_time;
                    key->long_press_triggered = false;  // 重置长按触发标志
                }
                break;
                
            case KEY_STATE_DEBOUNCING:
                if (current_pressed) {
                    // 消抖时间到，确认为按下
                    if (current_time - key->press_start_time >= key->config.debounce_time) {
                        key->state = KEY_STATE_PRESSED;
                        key->press_start_time = current_time;
                        
                        // 处理双击逻辑
                        if (current_time - key->last_release_time <= key->config.double_click_time) {
                            key->click_count++;
                            if (key->click_count >= 2) {
                                key->is_double_click = true;
                                key->click_count = 0;
                            }
                        } else {
                            key->click_count = 1;
                            key->is_double_click = false;
                        }
                    }
                } else {
                    // 消抖期间检测到释放，认为是抖动
                    key->state = KEY_STATE_RELEASED;
                }
                break;
                
            case KEY_STATE_PRESSED:
                if (!current_pressed) {
                    // 检测到释放，进入消抖状态
                    key->state = KEY_STATE_DEBOUNCING;
                    key->last_release_time = current_time;
                }
                break;
        }
    }
}

key_event_t key_manager_get_event(key_manager_t *manager, uint8_t key_index) {
    if (manager == NULL || key_index >= manager->key_count) {
        return KEY_EVENT_NONE;
    }
    
    key_info_t *key = &manager->keys[key_index];
    uint32_t current_time = get_current_time();
    uint32_t press_duration = current_time - key->press_start_time;
    
    // 状态变化检测
    if (key->state != key->last_state) {
        if (key->state == KEY_STATE_PRESSED && key->last_state != KEY_STATE_PRESSED) {
            return KEY_EVENT_PRESS;
        } else if (key->state == KEY_STATE_RELEASED && key->last_state != KEY_STATE_RELEASED) {
            // 检查是否为双击
            if (key->is_double_click) {
                key->is_double_click = false;
                return KEY_EVENT_DOUBLE_CLICK;
            }
            
            // 检查是否为长按（在释放时判断）
            if (press_duration >= key->config.long_press_time) {
                key->long_press_triggered = false;  // 重置长按触发标志
                return KEY_EVENT_LONG_PRESS;
            }
            
            // 检查是否为短按
            if (press_duration < key->config.long_press_time) {
                return KEY_EVENT_SHORT_PRESS;
            }
            
            return KEY_EVENT_RELEASE;
        }
    }
    
    // 保持按下状态检测（用于实时反馈，但不触发长按事件）
    if (key->state == KEY_STATE_PRESSED && press_duration >= key->config.long_press_time) {
        // 这里只返回HOLD事件，不触发长按事件
        return KEY_EVENT_HOLD;
    }
    
    // 保持按下状态
    if (key->state == KEY_STATE_PRESSED) {
        return KEY_EVENT_HOLD;
    }
    
    return KEY_EVENT_NONE;
}

bool key_manager_is_pressed(key_manager_t *manager, uint8_t key_index) {
    if (manager == NULL || key_index >= manager->key_count) {
        return false;
    }
    
    return (manager->keys[key_index].state == KEY_STATE_PRESSED);
}

bool key_manager_is_holding(key_manager_t *manager, uint8_t key_index) {
    if (manager == NULL || key_index >= manager->key_count) {
        return false;
    }
    
    key_info_t *key = &manager->keys[key_index];
    uint32_t current_time = get_current_time();
    uint32_t press_duration = current_time - key->press_start_time;
    
    return (key->state == KEY_STATE_PRESSED && press_duration >= key->config.long_press_time);
}

uint32_t key_manager_get_press_duration(key_manager_t *manager, uint8_t key_index) {
    if (manager == NULL || key_index >= manager->key_count) {
        return 0;
    }
    
    key_info_t *key = &manager->keys[key_index];
    
    if (key->state == KEY_STATE_PRESSED) {
        return get_current_time() - key->press_start_time;
    }
    
    return 0;
}

void key_manager_reset(key_manager_t *manager, uint8_t key_index) {
    if (manager == NULL || key_index >= manager->key_count) {
        return;
    }
    
    key_info_t *key = &manager->keys[key_index];
    key->state = KEY_STATE_RELEASED;
    key->last_state = KEY_STATE_RELEASED;
    key->press_start_time = 0;
    key->last_release_time = 0;
    key->click_count = 0;
    key->is_double_click = false;
    key->long_press_triggered = false;  // 新增：重置长按触发标志
}

const key_config_t* key_manager_get_config(key_manager_t *manager, uint8_t key_index) {
    if (manager == NULL || key_index >= manager->key_count) {
        return NULL;
    }
    
    return &manager->keys[key_index].config;
}

int8_t key_manager_set_config(key_manager_t *manager, uint8_t key_index,
                             uint32_t debounce_time, uint32_t long_press_time, uint32_t double_click_time) {
    if (manager == NULL || key_index >= manager->key_count) {
        return -1;
    }
    
    key_info_t *key = &manager->keys[key_index];
    
    if (debounce_time > 0) {
        key->config.debounce_time = debounce_time;
    }
    
    if (long_press_time > 0) {
        key->config.long_press_time = long_press_time;
    }
    
    if (double_click_time > 0) {
        key->config.double_click_time = double_click_time;
    }
    
    return 0;
}