#include "oled.h"
#include "oled_driver.h"
#include "songti_font.h"
#include <string.h>

// OLED初始化（兼容性接口）
void OLED_Init(void)
{
    oled_driver_init();
}

// OLED清屏（兼容性接口）
void OLED_Clear(void)
{
    oled_driver_clear();
}

// OLED刷新（兼容性接口）
void OLED_Refresh(void)
{
    oled_driver_refresh();
}

// 显示字符
void OLED_ShowChar(uint16_t x, uint16_t y, char chr, uint8_t size)
{
    if (size != 8) return; // 只支持8号字体
    
    const uint8_t* font_data = songti_font_get_char_data(chr);
    uint8_t font_width = songti_font_get_char_width(chr);
    uint8_t font_height = songti_font_get_char_height();
    
    for (uint8_t i = 0; i < font_height; i++) {
        uint8_t line_data = font_data[i];
        for (uint8_t j = 0; j < font_width; j++) {
            if (line_data & (1 << (7 - j))) {
                oled_driver_draw_pixel(x + j, y + i, true);
            }
        }
    }
}

// 显示字符串
void OLED_ShowString(uint16_t x, uint16_t y, const char* str, uint8_t size)
{
    uint16_t x_pos = x;
    while (*str) {
        OLED_ShowChar(x_pos, y, *str, size);
        x_pos += songti_font_get_char_width(*str) + 1;
        str++;
    }
}

// 显示数字
void OLED_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size)
{
    char str[20];
    snprintf(str, sizeof(str), "%*lu", len, num);
    OLED_ShowString(x, y, str, size);
}