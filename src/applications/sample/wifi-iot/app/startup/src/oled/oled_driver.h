#ifndef __OLED_DRIVER_H__
#define __OLED_DRIVER_H__

#include <stdint.h>
#include <stdbool.h>
#include "hi_io.h"
#include "hi_gpio.h"

#include "pin_definitions.h"

// OLED参数定义
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_PAGE_NUM 8
#define OLED_CMD 0x00
#define OLED_DATA 0x40

// 显示模式枚举
typedef enum {
    OLED_DISPLAY_NORMAL = 0,
    OLED_DISPLAY_INVERTED = 1
} oled_display_mode_t;

// OLED驱动结构体
typedef struct {
    uint8_t* gram;           // 前缓冲区
    uint8_t* back_buffer;    // 后缓冲区（可选）
    bool* dirty_region;      // 脏标记区域
    bool initialized;        // 初始化标志
    oled_display_mode_t display_mode; // 显示模式
} oled_driver_t;

// OLED驱动接口函数
int oled_driver_init(void);
void oled_driver_deinit(void);
void oled_driver_clear(void);
void oled_driver_clear_backbuffer(void);
void oled_driver_refresh(void);
void oled_driver_refresh_fast(void);
void oled_driver_refresh_partial(void);
void oled_driver_draw_pixel(uint16_t x, uint16_t y, bool set);
void oled_driver_draw_pixel_backbuffer(uint16_t x, uint16_t y, bool set);
void oled_driver_swap_buffers(void);
void oled_driver_set_display(bool on);
void oled_driver_set_display_mode(oled_display_mode_t mode);
bool oled_driver_is_initialized(void);
uint16_t oled_driver_get_width(void);
uint16_t oled_driver_get_height(void);

#endif // __OLED_DRIVER_H__