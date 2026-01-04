/**
 ****************************************************************************************************
 * @file        lightsense.h
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

#ifndef LIGHTSENSE_H
#define LIGHTSENSE_H

#include "cmsis_os2.h"
#include "hi_io.h"
#include "hi_gpio.h"

// 光敏传感器状态枚举
typedef enum {
    LIGHTSENSE_DARK = 0,    // 黑暗环境
    LIGHTSENSE_LIGHT = 1    // 明亮环境
} lightsense_state_t;

// 函数声明
void lightsense_init(void);
uint16_t lightsense_get_value(void);
float lightsense_get_voltage(void);
lightsense_state_t lightsense_get_state(void);
void lightsense_task(void);

#endif