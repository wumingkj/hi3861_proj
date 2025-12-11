#ifndef __BUZZER_H__
#define __BUZZER_H__

#include <stdint.h>

// 蜂鸣器初始化
void Buzzer_Init(void);

// 蜂鸣器鸣叫
void Buzzer_Beep(uint16_t duration_ms);

// 蜂鸣器模式鸣叫
void Buzzer_BeepPattern(uint16_t on_time, uint16_t off_time, uint8_t count);

#endif /* __BUZZER_H__ */