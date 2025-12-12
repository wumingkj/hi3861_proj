#include "buzzer.h"
#include "hi_io.h"
#include "hi_pwm.h"
#include <unistd.h>
#include "cmsis_os2.h"
#include <stdio.h>        // 添加printf支持
#include <stdbool.h>      // 添加bool类型支持

// 蜂鸣器引脚定义 - 使用GPIO14对应的PWM5端口
#define BEEP_PIN         HI_IO_NAME_GPIO_14
#define BEEP_PWM_FUN     HI_IO_FUNC_GPIO_14_PWM5_OUT
#define BEEP_PWM_PORT    HI_PWM_PORT_PWM5

// 蜂鸣器参数定义
#define BEEP_FREQ        2000    // 蜂鸣器频率 2kHz
#define BEEP_DUTY        500     // 蜂鸣器占空比 50%

// 蜂鸣器状态变量（非阻塞版本）
static uint32_t g_beep_start_time = 0;
static uint16_t g_beep_duration = 0;
static bool g_beep_active = false;

static uint32_t g_pattern_start_time = 0;
static uint16_t g_pattern_on_time = 0;
static uint16_t g_pattern_off_time = 0;
static uint8_t g_pattern_count = 0;
static uint8_t g_pattern_current = 0;
static bool g_pattern_on_phase = false;
static bool g_pattern_active = false;

// 蜂鸣器初始化
void Buzzer_Init(void)
{
    // 配置IO为PWM功能
    hi_io_set_pull(BEEP_PIN, HI_IO_PULL_UP);
    hi_io_set_func(BEEP_PIN, BEEP_PWM_FUN);
    
    // 初始化PWM
    hi_pwm_init(BEEP_PWM_PORT);
    hi_pwm_set_clock(PWM_CLK_160M);
    
    // 初始状态停止蜂鸣器
    hi_pwm_stop(BEEP_PWM_PORT);
    
    // 初始化状态变量
    g_beep_active = false;
    g_pattern_active = false;
    
    printf("Buzzer PWM Initialized Success (Non-blocking)!\n");
}

// 蜂鸣器鸣叫（非阻塞方式）
void Buzzer_Beep(uint16_t duration_ms)
{
    if (g_beep_active || g_pattern_active) {
        printf("Buzzer is busy, cannot start new beep\n");
        return;
    }
    
    // 启动PWM输出
    hi_pwm_start(BEEP_PWM_PORT, BEEP_DUTY, BEEP_FREQ);
    g_beep_start_time = osKernelGetTickCount();
    g_beep_duration = duration_ms;
    g_beep_active = true;
    
    printf("Buzzer Start Beep for %d ms (Non-blocking)\n", duration_ms);
}

// 蜂鸣器模式鸣叫（非阻塞方式）
void Buzzer_BeepPattern(uint16_t on_time, uint16_t off_time, uint8_t count)
{
    if (g_beep_active || g_pattern_active) {
        printf("Buzzer is busy, cannot start new pattern\n");
        return;
    }
    
    g_pattern_start_time = osKernelGetTickCount();
    g_pattern_on_time = on_time;
    g_pattern_off_time = off_time;
    g_pattern_count = count;
    g_pattern_current = 0;
    g_pattern_on_phase = true;
    g_pattern_active = true;
    
    // 启动第一个鸣叫周期
    hi_pwm_start(BEEP_PWM_PORT, BEEP_DUTY, BEEP_FREQ);
    
    printf("Buzzer Pattern: on=%dms, off=%dms, count=%d (Non-blocking)\n", 
           on_time, off_time, count);
}

// 停止蜂鸣器（立即停止）
void Buzzer_Stop(void)
{
    hi_pwm_stop(BEEP_PWM_PORT);
    g_beep_active = false;
    g_pattern_active = false;
    printf("Buzzer Stopped Immediately\n");
}

// 检查蜂鸣器是否正在鸣叫
uint8_t Buzzer_IsActive(void)
{
    return (g_beep_active || g_pattern_active) ? 1 : 0;
}

// 蜂鸣器更新函数（非阻塞版本）
void Buzzer_Update(void)
{
    uint32_t current_time = osKernelGetTickCount();
    
    // 处理单次鸣叫
    if (g_beep_active) {
        if (current_time - g_beep_start_time >= g_beep_duration) {
            hi_pwm_stop(BEEP_PWM_PORT);
            g_beep_active = false;
            printf("Buzzer Beep Finished\n");
        }
    }
    
    // 处理模式鸣叫
    if (g_pattern_active) {
        uint32_t elapsed = current_time - g_pattern_start_time;
        
        if (g_pattern_on_phase) {
            // 鸣叫阶段
            if (elapsed >= g_pattern_on_time) {
                // 切换到停止阶段
                hi_pwm_stop(BEEP_PWM_PORT);
                g_pattern_start_time = current_time;
                g_pattern_on_phase = false;
            }
        } else {
            // 停止阶段
            if (elapsed >= g_pattern_off_time) {
                // 检查是否完成所有周期
                g_pattern_current++;
                if (g_pattern_current >= g_pattern_count) {
                    // 完成所有周期
                    g_pattern_active = false;
                    printf("Buzzer Pattern Finished\n");
                } else {
                    // 开始下一个周期
                    hi_pwm_start(BEEP_PWM_PORT, BEEP_DUTY, BEEP_FREQ);
                    g_pattern_start_time = current_time;
                    g_pattern_on_phase = true;
                }
            }
        }
    }
}