#ifndef __BUZZER_H__
#define __BUZZER_H__

#include <stdint.h>

// 蜂鸣器初始化
void Buzzer_Init(void);

// 蜂鸣器鸣叫（非阻塞方式）
void Buzzer_Beep(uint16_t duration_ms);

// 蜂鸣器模式鸣叫（非阻塞方式）
void Buzzer_BeepPattern(uint16_t on_time, uint16_t off_time, uint8_t count);

// 蜂鸣器更新函数（需要在主循环中调用）
void Buzzer_Update(void);

// 停止蜂鸣器（立即停止）
void Buzzer_Stop(void);

// 检查蜂鸣器是否正在鸣叫
uint8_t Buzzer_IsActive(void);

#endif // __BUZZER_H__