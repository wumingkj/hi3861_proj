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
    uint8_t* gram;           // 前缓冲区（当前显示内容）
    uint8_t* back_buffer;    // 后缓冲区（绘制目标）
    uint8_t* saved_gram;     // 渲染时暂存前缓冲区指针
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
void oled_driver_begin_render(void);
void oled_driver_begin_render_copy(void);
void oled_driver_end_render(void);
void oled_driver_set_display(bool on);
void oled_driver_set_display_mode(oled_display_mode_t mode);
void oled_driver_set_contrast(uint8_t contrast);
bool oled_driver_is_initialized(void);
uint16_t oled_driver_get_width(void);
uint16_t oled_driver_get_height(void);

// 直接写入显存（页格式，每字节 = 8 个垂直像素）
// data 按页排列: data[page * w + col], page = 0..((h+7)/8 - 1)
void oled_driver_write_gram_block(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data);
void oled_driver_write_gram_block_from_bitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *bitmap);

#endif // __OLED_DRIVER_H__