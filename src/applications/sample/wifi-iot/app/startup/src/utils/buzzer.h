#ifndef BUZZER_H
#define BUZZER_H

#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "hi_gpio.h"

// 蜂鸣器GPIO引脚定义 - 根据硬件连接修改
#define BUZZER_GPIO_IDX HI_GPIO_IDX_14  // 注意：使用枚举值

// 蜂鸣器状态
typedef enum {
    BUZZER_OFF = HI_GPIO_VALUE0,
    BUZZER_ON = HI_GPIO_VALUE1
} BuzzerState;

// 函数声明
void Buzzer_Init(void);
void Buzzer_Beep(uint32_t duration_ms);
void Buzzer_BeepPattern(uint32_t beep_ms, uint32_t silence_ms, uint8_t times);
void Buzzer_SetState(BuzzerState state);
void Buzzer_Deinit(void);

#endif // BUZZER_H