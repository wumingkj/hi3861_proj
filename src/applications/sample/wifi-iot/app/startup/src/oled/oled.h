#ifndef __OLED_H__
#define __OLED_H__

#include <stdint.h>
#include <stdbool.h>

// 兼容性接口
void OLED_Init(void);
void OLED_Clear(void);
void OLED_Refresh(void);
void OLED_ShowChar(uint16_t x, uint16_t y, char chr, uint8_t size);
void OLED_ShowString(uint16_t x, uint16_t y, const char* str, uint8_t size);
void OLED_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size);

#endif /* __OLED_H__ */