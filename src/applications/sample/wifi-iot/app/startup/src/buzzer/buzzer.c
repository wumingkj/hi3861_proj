#include "buzzer.h"
#include "hi_types_base.h"
#include "hi_io.h"
#include "hi_pwm.h"
#include <stdio.h>
#include <unistd.h>

// 蜂鸣器引脚配置 - 使用Hi3861开发板支持的GPIO引脚
#define BUZZER_PWM_ID      HI_PWM_PORT_PWM5        // 使用PWM0通道
#define BUZZER_GPIO_PIN    HI_IO_NAME_GPIO_14       // 使用GPIO9引脚
#define BUZZER_PWM_FUNC    HI_IO_FUNC_GPIO_14_PWM5_OUT  // GPIO9的PWM0输出功能

// 蜂鸣器初始化
void Buzzer_Init(void)
{
    // 设置GPIO引脚功能为PWM
    hi_io_set_func(BUZZER_GPIO_PIN, BUZZER_PWM_FUNC);
    
    // 初始化PWM
    hi_pwm_init(BUZZER_PWM_ID);
    
    printf("Buzzer Initialized Success!\n");
}

// 蜂鸣器鸣叫
void Buzzer_Beep(uint16_t duration_ms)
{
    // 设置PWM频率为2kHz，占空比50%
    hi_pwm_set_clock(PWM_CLK_160M);
    hi_pwm_start(BUZZER_PWM_ID, 2000, 5000); // 2kHz, 50% duty
    
    // 使用usleep进行延时（毫秒转微秒）
    usleep(duration_ms * 1000);
    
    // 停止PWM
    hi_pwm_stop(BUZZER_PWM_ID);
}

// 蜂鸣器模式鸣叫
void Buzzer_BeepPattern(uint16_t on_time, uint16_t off_time, uint8_t count)
{
    for (uint8_t i = 0; i < count; i++) {
        Buzzer_Beep(on_time);
        usleep(off_time * 1000);
    }
}