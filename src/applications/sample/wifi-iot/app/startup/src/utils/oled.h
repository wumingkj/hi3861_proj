#ifndef __OLED_H__
#define __OLED_H__

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "hi_gpio.h"
#include "hi_i2c.h"
#include "hi_io.h"
#include "hi_early_debug.h"

// 添加标准整数类型定义
#include <stdint.h>

// OLED参数
#define OLED_I2C_IDX        0           // I2C通道号
#define OLED_ADDRESS        0x78        // OLED I2C地址（有的可能是0x3C，需要左移一位）
#define OLED_WIDTH          128         // OLED宽度
#define OLED_HEIGHT         64          // OLED高度
#define OLED_PAGE_NUM       8           // OLED页数（64/8=8）

// I2C引脚配置 - 使用Hi3861默认的I2C0引脚
#define OLED_I2C_SCL_PIN    9           // I2C时钟引脚 GPIO9
#define OLED_I2C_SDA_PIN    10          // I2C数据引脚 GPIO10

// OLED命令定义
#define OLED_CMD             0x00        // 写命令
#define OLED_DATA            0x40        // 写数据

// 基本命令
#define OLED_SET_DISP_ON     0xAF        // 开启显示
#define OLED_SET_DISP_OFF    0xAE        // 关闭显示

// 函数声明
void OLED_Init(void);
void OLED_Clear(void);
void OLED_Display_On(void);
void OLED_Display_Off(void);
void OLED_Refresh(void);
void OLED_SetPos(uint16_t x, uint16_t y);
void OLED_DrawPoint(uint16_t x, uint16_t y, uint16_t mode);
void OLED_ShowChar(uint16_t x, uint16_t y, char chr, uint16_t size);
void OLED_ShowString(uint16_t x, uint16_t y, char *str, uint16_t size);
void OLED_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint16_t len, uint16_t size);

#endif /* __OLED_H__ */