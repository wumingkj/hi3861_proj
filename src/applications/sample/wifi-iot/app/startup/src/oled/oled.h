#ifndef __OLED_H__
#define __OLED_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

// 刷新模式：轮询（省资源） vs 事件驱动（零CPU占用）
typedef enum {
    OLED_REFRESH_POLLING = 0,   // 定时轮询脏标记（默认，兼容性好）
    OLED_REFRESH_EVENT   = 1    // 事件驱动，无更新时任务挂起（省CPU）
} oled_refresh_mode_t;

// OLED初始化函数（选择刷新模式）
int OLED_Init(oled_refresh_mode_t mode);
void OLED_Deinit(void);
void OLED_Clear(void);
void OLED_Refresh(void);
bool OLED_IsReady(void);

// 显示控制
void OLED_Invert(bool invert);           // true=翻转（黑底白字），false=正常（白底黑字）
void OLED_SetContrast(uint8_t contrast); // 对比度 0~255
void OLED_DisplayOn(bool on);            // 屏幕开关（不断电，仅关闭/开启显示）

// printf风格显示（缓冲区模式，自动换行，支持\n）
// 调用后存到内部缓冲区，后台任务自动渲染到屏幕
// 例: OLED_Printf(16, "温度: %.1f°C\n湿度: %d%%", temp, hum);
void OLED_Printf(uint8_t size, const char* fmt, ...);

// 显示图片（页格式数据，每字节=8垂直像素，SSD1306原生格式）
// 图片数据可用 tools/image_to_oled.py 转换
void OLED_ShowImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data);

// 显示水平 bitmap（行优先格式，每字节=8水平像素，MSB左）
// 兼容网上最常见的 "128x64 mono bitmap" 格式
void OLED_ShowBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *bitmap);

// 直接叠印文本到屏幕（不经过缓冲区，不擦除已有内容）
// 适合在图片/画面上叠文字
void OLED_DrawText(uint16_t x, uint16_t y, uint8_t size, const char* str);

#endif // __OLED_H__