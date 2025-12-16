#ifndef __OLED_H__
#define __OLED_H__

#include <stdint.h>
#include <stdbool.h>

// OLED初始化函数
int OLED_Init(void);
void OLED_Deinit(void);
void OLED_Clear(void);
void OLED_Refresh(void);
bool OLED_IsReady(void);

// 显示函数
void OLED_ShowString(uint16_t x, uint16_t y, const char* str, uint8_t size);
void OLED_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size);
void OLED_ShowTemperatureHumidity(float temperature, int humidity);

#endif // __OLED_H__