#ifndef __TIME_H__
#define __TIME_H__

#include <stdint.h>
#include <stdbool.h>
#include "cmsis_os2.h"

// 系统时间初始化
void Time_Init(void);

// 获取当前时间（毫秒）
uint32_t Time_GetCurrentMs(void);

// 获取系统tick频率
uint32_t Time_GetTickFrequency(void);

// 任务延时函数（使用osDelay，适合长延时）
void Time_DelayMs(uint32_t ms);

// 精确延时函数（使用usleep，适合短延时）
void Time_DelayMsPrecise(uint32_t ms);

// 检查是否达到时间间隔
bool Time_CheckInterval(uint32_t *last_time, uint32_t interval_ms);

#endif /* __TIME_H__ */