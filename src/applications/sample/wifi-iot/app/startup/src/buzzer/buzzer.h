#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include <stdbool.h>
#include "hi_io.h"
#include "hi_gpio.h"
#include "hi_pwm.h"

#include "debug.h"   // 添加debug库支持
#include "pin_definitions.h"

// 可调：频率由 period 决定；示例给个“听得到”的值
#define BEEP_PWM_PERIOD       40000
#define BEEP_PWM_DUTY         20000

// 音符频率定义（Hz）
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523


// 初始化PWM蜂鸣器硬件
void Buzzer_Init(void);

// 1) 叫 ms 毫秒（非阻塞，立即返回）
//    会打断当前播放
void Buzzer_BeepMs(uint16_t ms);

// 2) 报警模式：响 on_ms、停 off_ms，重复 cnt 次（非阻塞）
//    会打断当前播放
void Buzzer_Alarm(uint16_t cnt, uint16_t on_ms, uint16_t off_ms);

// 3) 新的PWM控制API：设置频率和占空比
void Buzzer_SetFrequency(uint32_t freq_hz);
void Buzzer_SetDuty(uint16_t duty);
void Buzzer_StartPWM(void);
void Buzzer_StopPWM(void);

// 4) 播放小星星音乐
void Buzzer_PlayLittleStar(void);

// 5) 立刻停止（随时可调用）
void Buzzer_Stop(void);

// 6) 是否正在播放（响或停间隙都算忙）
bool Buzzer_IsBusy(void);

// 7) 给 main 的定时器回调调用：每 1ms/5ms 调一次都可以
void Buzzer_Tick(uint32_t now_ms);

#endif