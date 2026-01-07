/**
 ****************************************************************************************************
 * @file        relay.h
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
 * SRD-05VDC-SL-C继电器参数：
 * - 工作电压：5V DC
 * - 控制电压：5V DC
 * - 最大负载：10A 250V AC / 10A 30V DC
 * - 触发电流：约20mA
 *
 */

#ifndef RELAY_H
#define RELAY_H

#include "cmsis_os2.h"
#include "hi_io.h"
#include "hi_gpio.h"

#include "pin_definitions.h"
// 继电器状态枚举
typedef enum {
    RELAY_OFF = 0,    // 继电器断开
    RELAY_ON = 1      // 继电器闭合
} relay_state_t;


// 函数声明
void relay_init(void);
void relay_set_state(relay_state_t state);
relay_state_t relay_get_state(void);
void relay_toggle(void);
void relay_task(void);

#endif