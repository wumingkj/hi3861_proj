#include "buzzer.h"
#include "hi_io.h"
#include "hi_pwm.h"
#include <unistd.h>
#include "cmsis_os2.h"

// 蜂鸣器引脚定义 - 使用GPIO14对应的PWM5端口
#define BEEP_PIN         HI_IO_NAME_GPIO_14
#define BEEP_PWM_FUN     HI_IO_FUNC_GPIO_14_PWM5_OUT
#define BEEP_PWM_PORT    HI_PWM_PORT_PWM5

// 蜂鸣器参数定义
#define BEEP_FREQ        2000    // 蜂鸣器频率 2kHz
#define BEEP_DUTY        500     // 蜂鸣器占空比 50%

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
    
    printf("Buzzer PWM Initialized Success!\n");
}

// 蜂鸣器鸣叫（阻塞方式 - 简单直接）
void Buzzer_Beep(uint16_t duration_ms)
{
    // 启动PWM输出
    hi_pwm_start(BEEP_PWM_PORT, BEEP_DUTY, BEEP_FREQ);
    printf("Buzzer Start Beep for %d ms\n", duration_ms);
    
    // 阻塞延时
    usleep(duration_ms * 1000);
    
    // 停止PWM输出
    hi_pwm_stop(BEEP_PWM_PORT);
    printf("Buzzer Stop\n");
}

// 蜂鸣器模式鸣叫（阻塞方式）
void Buzzer_BeepPattern(uint16_t on_time, uint16_t off_time, uint8_t count)
{
    printf("Buzzer Pattern: on=%dms, off=%dms, count=%d\n", on_time, off_time, count);
    
    for (uint8_t i = 0; i < count; i++) {
        // 鸣叫
        hi_pwm_start(BEEP_PWM_PORT, BEEP_DUTY, BEEP_FREQ);
        usleep(on_time * 1000);
        
        // 停止
        hi_pwm_stop(BEEP_PWM_PORT);
        usleep(off_time * 1000);
    }
    printf("Buzzer Pattern Finished\n");
}

// 停止蜂鸣器（立即停止）
void Buzzer_Stop(void)
{
    hi_pwm_stop(BEEP_PWM_PORT);
    printf("Buzzer Stopped Immediately\n");
}

// 检查蜂鸣器是否正在鸣叫（阻塞方式下总是返回0）
uint8_t Buzzer_IsActive(void)
{
    return 0;
}

// 蜂鸣器更新函数（阻塞方式下不需要）
void Buzzer_Update(void)
{
    // 阻塞方式下不需要更新
}