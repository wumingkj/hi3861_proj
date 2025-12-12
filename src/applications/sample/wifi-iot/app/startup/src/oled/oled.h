#ifndef __OLED_H__
#define __OLED_H__

#include <stdint.h>
#include <stdbool.h>

// OLED初始化（包含后台刷新任务）
void OLED_Init(void);
void OLED_Deinit(void);

// 清屏接口
void OLED_Clear(void);

// 显示请求接口（非阻塞）
void OLED_RequestShowString(uint16_t x, uint16_t y, const char* str, uint8_t size);
void OLED_RequestShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size);

// 强制刷新接口（可选）
void OLED_ForceRefresh(void);

// 获取OLED状态
bool OLED_IsReady(void);

#endif /* __OLED_H__ */