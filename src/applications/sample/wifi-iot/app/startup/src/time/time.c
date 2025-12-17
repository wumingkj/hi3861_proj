#include "time.h"
#include <stdio.h>
#include <unistd.h>  // 添加usleep支持

static uint32_t g_tick_freq = 0;

// 系统时间初始化
void Time_Init(void)
{
    g_tick_freq = osKernelGetTickFreq();
    log_i("TIME", "Time Library Init: Tick Frequency = %d Hz", g_tick_freq);
}

// 获取当前时间（毫秒）
uint32_t Time_GetCurrentMs(void)
{
    if (g_tick_freq == 0) {
        return 0;
    }
    return (osKernelGetTickCount() * 1000) / g_tick_freq;
}

// 获取系统tick频率
uint32_t Time_GetTickFrequency(void)
{
    return g_tick_freq;
}

// 任务延时函数（使用osDelay，适合长延时）
void Time_DelayMs(uint32_t ms)
{
    if (g_tick_freq == 0) {
        return;
    }
    
    uint32_t ticks = (ms * g_tick_freq) / 1000;
    if (ticks == 0) {
        ticks = 1;  // 至少延时1个tick
    }
    osDelay(ticks);
}

// 精确延时函数（使用usleep，适合短延时）
void Time_DelayMsPrecise(uint32_t ms)
{
    if (ms == 0) {
        return;
    }
    
    // 将毫秒转换为微秒
    uint32_t microseconds = ms * 1000;
    usleep(microseconds);
}

// 检查是否达到时间间隔
bool Time_CheckInterval(uint32_t *last_time, uint32_t interval_ms)
{
    uint32_t current_time = Time_GetCurrentMs();
    if (current_time - *last_time >= interval_ms) {
        *last_time = current_time;
        return true;
    }
    return false;
}