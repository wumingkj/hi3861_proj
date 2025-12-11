#include "buzzer.h"
#include "hi_io.h"
#include "hi_pwm.h"
#include <unistd.h>
#include "cmsis_os2.h"

// 蜂鸣器引脚定义 - 使用GPIO14对应的PWM5端口
#define BEEP_PIN         HI_IO_NAME_GPIO_7
#define BEEP_PWM_FUN     HI_IO_FUNC_GPIO_7_PWM0_OUT
#define BEEP_PWM_PORT    HI_PWM_PORT_PWM0

// 蜂鸣器参数定义
#define BEEP_FREQ        2000    // 蜂鸣器频率 2kHz
#define BEEP_DUTY        500     // 蜂鸣器占空比 50%

// 蜂鸣器状态变量
static volatile uint8_t g_beep_active = 0;
static volatile uint32_t g_beep_end_time = 0;

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

// 蜂鸣器鸣叫（非阻塞方式）
void Buzzer_Beep(uint16_t duration_ms)
{
    // 启动PWM输出
    hi_pwm_start(BEEP_PWM_PORT, BEEP_DUTY, BEEP_FREQ);
    
    // 设置蜂鸣结束时间（非阻塞方式）
    g_beep_active = 1;
    g_beep_end_time = osKernelGetTickCount() + duration_ms;
    
    printf("Buzzer Beep for %d ms\n", duration_ms);
}

// 蜂鸣器模式鸣叫（非阻塞方式）
void Buzzer_BeepPattern(uint16_t on_time, uint16_t off_time, uint8_t count)
{
    // 简化实现：使用单次蜂鸣代替模式
    // 在实际应用中，可以使用状态机实现非阻塞的模式控制
    Buzzer_Beep(on_time);
    printf("Buzzer Pattern: on=%dms, off=%dms, count=%d\n", on_time, off_time, count);
}

// 蜂鸣器更新函数（需要在主循环中调用）
void Buzzer_Update(void)
{
    if (g_beep_active) {
        uint32_t current_time = osKernelGetTickCount();
        if (current_time >= g_beep_end_time) {
            // 停止蜂鸣器
            hi_pwm_stop(BEEP_PWM_PORT);
            g_beep_active = 0;
            printf("Buzzer Stopped\n");
        }
    }
}

// 停止蜂鸣器（立即停止）
void Buzzer_Stop(void)
{
    hi_pwm_stop(BEEP_PWM_PORT);
    g_beep_active = 0;
    printf("Buzzer Stopped Immediately\n");
}

// 检查蜂鸣器是否正在鸣叫
uint8_t Buzzer_IsActive(void)
{
    return g_beep_active;
}