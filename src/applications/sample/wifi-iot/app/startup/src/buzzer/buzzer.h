#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include <stdbool.h>
#include "hi_io.h"
#include "hi_gpio.h"
#include "hi_pwm.h"

#define BEEP_PIN              HI_IO_NAME_GPIO_14
#define BEEP_PIN_PWM_FUNC     HI_IO_FUNC_GPIO_14_PWM5_OUT
#define BEEP_PWM_PORT         HI_PWM_PORT_PWM5

// 可调：频率由 period 决定；示例给个“听得到”的值
#define BEEP_PWM_PERIOD       40000
#define BEEP_PWM_DUTY         20000

// 初始化PWM蜂鸣器硬件
void Buzzer_Init(void);

// 1) 叫 ms 毫秒（非阻塞，立即返回）
//    会打断当前播放
void Buzzer_BeepMs(uint16_t ms);

// 2) 报警模式：响 on_ms、停 off_ms，重复 cnt 次（非阻塞）
//    会打断当前播放
void Buzzer_Alarm(uint16_t cnt, uint16_t on_ms, uint16_t off_ms);

// 3) 立刻停止（随时可调用）
void Buzzer_Stop(void);

// 4) 是否正在播放（响或停间隙都算忙）
bool Buzzer_IsBusy(void);

// 5) 给 main 的定时器回调调用：每 1ms/5ms 调一次都可以
void Buzzer_Tick(uint32_t now_ms);

#endif
