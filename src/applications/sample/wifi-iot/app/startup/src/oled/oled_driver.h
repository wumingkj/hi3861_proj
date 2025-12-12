#ifndef __OLED_DRIVER_H__
#define __OLED_DRIVER_H__

#include <stdint.h>
#include <stdbool.h>

// OLED参数配置
#define OLED_I2C_IDX        0           // I2C通道号
#define OLED_ADDRESS        0x78        // OLED I2C地址
#define OLED_WIDTH          128         // OLED宽度
#define OLED_HEIGHT         64          // OLED高度
#define OLED_PAGE_NUM       8           // OLED页数

// I2C引脚配置
#define OLED_I2C_SCL_PIN    HI_IO_FUNC_GPIO_9_I2C0_SCL           // I2C时钟引脚 GPIO9
#define OLED_I2C_SDA_PIN    HI_IO_FUNC_GPIO_10_I2C0_SDA          // I2C数据引脚 GPIO10

// OLED命令定义
#define OLED_CMD            0x00        // 写命令
#define OLED_DATA           0x40        // 写数据

// 显示模式
typedef enum {
    OLED_DISPLAY_NORMAL = 0,
    OLED_DISPLAY_INVERTED
} oled_display_mode_t;

// OLED驱动结构体
typedef struct {
    uint8_t gram[OLED_WIDTH][OLED_PAGE_NUM];  // 前缓冲区
    uint8_t back_buffer[OLED_WIDTH][OLED_PAGE_NUM];  // 后缓冲区
    bool initialized;                         // 初始化标志
    oled_display_mode_t display_mode;         // 显示模式
} oled_driver_t;

// 初始化函数
int oled_driver_init(void);
void oled_driver_deinit(void);

// 基本操作
void oled_driver_clear(void);
void oled_driver_refresh(void);
void oled_driver_refresh_fast(void);
void oled_driver_refresh_partial(void);
void oled_driver_set_display(bool on);
void oled_driver_set_display_mode(oled_display_mode_t mode);

// 像素操作
void oled_driver_draw_pixel(uint16_t x, uint16_t y, bool set);
void oled_driver_draw_pixel_backbuffer(uint16_t x, uint16_t y, bool set);

// 双缓冲操作
void oled_driver_swap_buffers(void);
void oled_driver_clear_backbuffer(void);

// 获取驱动状态
bool oled_driver_is_initialized(void);
uint16_t oled_driver_get_width(void);
uint16_t oled_driver_get_height(void);

#endif /* __OLED_DRIVER_H__ */