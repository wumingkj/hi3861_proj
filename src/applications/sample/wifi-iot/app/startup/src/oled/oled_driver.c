#include "oled_driver.h"
#include "hi_io.h"
#include "hi_gpio.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>  // 添加malloc/free支持

// 全局驱动实例 - 改为指针
static oled_driver_t* g_oled_driver = NULL;

// 软件I2C起始信号
static void sw_i2c_start(void) {
    hi_gpio_set_dir(OLED_SW_I2C_SDA_PIN, HI_GPIO_DIR_OUT);
    hi_gpio_set_dir(OLED_SW_I2C_SCL_PIN, HI_GPIO_DIR_OUT);
    
    hi_gpio_set_ouput_val(OLED_SW_I2C_SDA_PIN, 1);
    hi_gpio_set_ouput_val(OLED_SW_I2C_SCL_PIN, 1);
    
    hi_gpio_set_ouput_val(OLED_SW_I2C_SDA_PIN, 0);
    
    hi_gpio_set_ouput_val(OLED_SW_I2C_SCL_PIN, 0);
}

// 软件I2C停止信号
static void sw_i2c_stop(void)
{
    hi_gpio_set_dir(OLED_SW_I2C_SDA_PIN, HI_GPIO_DIR_OUT);
    hi_gpio_set_dir(OLED_SW_I2C_SCL_PIN, HI_GPIO_DIR_OUT);
    
    hi_gpio_set_ouput_val(OLED_SW_I2C_SDA_PIN, 0);
    hi_gpio_set_ouput_val(OLED_SW_I2C_SCL_PIN, 0);
    
    hi_gpio_set_ouput_val(OLED_SW_I2C_SCL_PIN, 1);
    
    hi_gpio_set_ouput_val(OLED_SW_I2C_SDA_PIN, 1);
}

// 软件I2C写字节
// 在文件开头添加必要的宏定义
#ifndef HI_GPIO_OK
#define HI_GPIO_OK 0  // 定义HI_GPIO_OK宏，如果头文件中没有定义
#endif

// 或者直接使用0代替HI_GPIO_OK
// 修改sw_i2c_write_byte函数中的ACK检查部分
static void sw_i2c_write_byte(uint8_t data)
{
    hi_gpio_set_dir(OLED_SW_I2C_SDA_PIN, HI_GPIO_DIR_OUT);
    hi_gpio_set_dir(OLED_SW_I2C_SCL_PIN, HI_GPIO_DIR_OUT);
    
    for (int i = 0; i < 8; i++) {
        hi_gpio_set_ouput_val(OLED_SW_I2C_SDA_PIN, (data & 0x80) ? 1 : 0);
        
        hi_gpio_set_ouput_val(OLED_SW_I2C_SCL_PIN, 1);
        
        hi_gpio_set_ouput_val(OLED_SW_I2C_SCL_PIN, 0);
        
        data <<= 1;
    }
    
    // 等待ACK（简化处理，不检查返回值）
    hi_gpio_set_dir(OLED_SW_I2C_SDA_PIN, HI_GPIO_DIR_IN);
    hi_gpio_set_ouput_val(OLED_SW_I2C_SCL_PIN, 1);
    hi_gpio_set_ouput_val(OLED_SW_I2C_SCL_PIN, 0);
}

// 写命令到OLED（软件I2C版本）
static void oled_write_cmd(uint8_t cmd)
{
    sw_i2c_start();
    sw_i2c_write_byte(0x78); // OLED地址（写模式）
    sw_i2c_write_byte(OLED_CMD);
    sw_i2c_write_byte(cmd);
    sw_i2c_stop();
}

// 写数据到OLED（软件I2C版本）
static void oled_write_data(uint8_t data)
{
    sw_i2c_start();
    sw_i2c_write_byte(0x78); // OLED地址（写模式）
    sw_i2c_write_byte(OLED_DATA);
    sw_i2c_write_byte(data);
    sw_i2c_stop();
}

// OLED初始化 - 软件I2C版本
int oled_driver_init(void)
{
    // 检查是否已经初始化
    if (g_oled_driver != NULL && g_oled_driver->initialized) {
        return 0;
    }
    
    // 分配驱动结构体内存
    g_oled_driver = (oled_driver_t*)malloc(sizeof(oled_driver_t));
    if (g_oled_driver == NULL) {
        printf("OLED Driver: Failed to allocate memory for driver structure!\n");
        return -1;
    }
    
    // 分配前缓冲区内存
    g_oled_driver->gram = (uint8_t*)malloc(OLED_WIDTH * OLED_PAGE_NUM * sizeof(uint8_t));
    if (g_oled_driver->gram == NULL) {
        printf("OLED Driver: Failed to allocate memory for front buffer!\n");
        free(g_oled_driver);
        g_oled_driver = NULL;
        return -1;
    }
    
    // 分配后缓冲区内存（可选，可以注释掉以节省内存）
    g_oled_driver->back_buffer = (uint8_t*)malloc(OLED_WIDTH * OLED_PAGE_NUM * sizeof(uint8_t));
    if (g_oled_driver->back_buffer == NULL) {
        printf("OLED Driver: Failed to allocate memory for back buffer!\n");
        free(g_oled_driver->gram);
        free(g_oled_driver);
        g_oled_driver = NULL;
        return -1;
    }
    
    // 分配脏标记区域内存
    g_oled_driver->dirty_region = (bool*)malloc(OLED_PAGE_NUM * sizeof(bool));
    if (g_oled_driver->dirty_region == NULL) {
        printf("OLED Driver: Failed to allocate memory for dirty region!\n");
        free(g_oled_driver->back_buffer);
        free(g_oled_driver->gram);
        free(g_oled_driver);
        g_oled_driver = NULL;
        return -1;
    }
    
    // 初始化软件I2C引脚
    hi_gpio_init();
    hi_io_set_pull(OLED_SW_I2C_SCL_PIN, HI_IO_PULL_UP);
    hi_io_set_pull(OLED_SW_I2C_SDA_PIN, HI_IO_PULL_UP);
    hi_io_set_func(OLED_SW_I2C_SCL_PIN, HI_IO_FUNC_GPIO_9_GPIO);
    hi_io_set_func(OLED_SW_I2C_SDA_PIN, HI_IO_FUNC_GPIO_10_GPIO);
    hi_gpio_set_dir(OLED_SW_I2C_SCL_PIN, HI_GPIO_DIR_OUT);
    hi_gpio_set_dir(OLED_SW_I2C_SDA_PIN, HI_GPIO_DIR_OUT);
    
    // OLED初始化序列
    oled_write_cmd(0xAE); // 关闭显示
    oled_write_cmd(0x20); // 设置内存地址模式
    oled_write_cmd(0x10); // 页地址模式
    oled_write_cmd(0xB0); // 设置页起始地址
    oled_write_cmd(0xC8); // 设置COM扫描方向
    oled_write_cmd(0x00); // 设置列地址低4位
    oled_write_cmd(0x10); // 设置列地址高4位
    oled_write_cmd(0x40); // 设置显示起始行
    oled_write_cmd(0x81); // 设置对比度
    oled_write_cmd(0xFF); // 对比度值
    oled_write_cmd(0xA1); // 设置段重映射
    oled_write_cmd(0xA6); // 设置正常显示
    oled_write_cmd(0xA8); // 设置多路复用率
    oled_write_cmd(0x3F); // 1/64 duty
    oled_write_cmd(0xA4); // 输出跟随RAM内容
    oled_write_cmd(0xD3); // 设置显示偏移
    oled_write_cmd(0x00); // 无偏移
    oled_write_cmd(0xD5); // 设置显示时钟分频比/振荡器频率
    oled_write_cmd(0xF0); // 设置分频比
    oled_write_cmd(0xD9); // 设置预充电周期
    oled_write_cmd(0x22); // 
    oled_write_cmd(0xDA); // 设置COM引脚硬件配置
    oled_write_cmd(0x12); // 
    oled_write_cmd(0xDB); // 设置VCOMH
    oled_write_cmd(0x20); // 0.77xVcc
    oled_write_cmd(0x8D); // 设置电荷泵
    oled_write_cmd(0x14); // 启用电荷泵
    oled_write_cmd(0xAF); // 开启显示
    
    // 清屏
    oled_driver_clear();
    oled_driver_refresh();
    
    g_oled_driver->initialized = true;
    g_oled_driver->display_mode = OLED_DISPLAY_NORMAL;
    
    printf("OLED Driver Initialized Success with Software I2C! (Heap Memory: %d bytes)\n", 
           sizeof(oled_driver_t) + (OLED_WIDTH * OLED_PAGE_NUM * 2) + OLED_PAGE_NUM);
    return 0;
}

void oled_driver_deinit(void)
{
    if (g_oled_driver == NULL) {
        return;
    }
    
    if (g_oled_driver->initialized) {
        oled_write_cmd(0xAE); // 关闭显示
    }
    
    // 释放所有堆内存
    if (g_oled_driver->dirty_region != NULL) {
        free(g_oled_driver->dirty_region);
    }
    if (g_oled_driver->back_buffer != NULL) {
        free(g_oled_driver->back_buffer);
    }
    if (g_oled_driver->gram != NULL) {
        free(g_oled_driver->gram);
    }
    free(g_oled_driver);
    g_oled_driver = NULL;
    
    printf("OLED Driver Deinitialized Success!\n");
}

// 清屏
void oled_driver_clear(void)
{
    if (g_oled_driver == NULL || !g_oled_driver->initialized) {
        return;
    }
    
    memset(g_oled_driver->gram, 0, OLED_WIDTH * OLED_PAGE_NUM);
    if (g_oled_driver->back_buffer != NULL) {
        memset(g_oled_driver->back_buffer, 0, OLED_WIDTH * OLED_PAGE_NUM);
    }
    memset(g_oled_driver->dirty_region, 0, OLED_PAGE_NUM);
}

// 清空后缓冲区
void oled_driver_clear_backbuffer(void)
{
    if (g_oled_driver == NULL || !g_oled_driver->initialized || g_oled_driver->back_buffer == NULL) {
        return;
    }
    
    memset(g_oled_driver->back_buffer, 0, OLED_WIDTH * OLED_PAGE_NUM);
}

// 刷新显示（原始版本）
void oled_driver_refresh(void)
{
    if (g_oled_driver == NULL || !g_oled_driver->initialized) {
        return;
    }
    
    for (uint8_t page = 0; page < OLED_PAGE_NUM; page++) {
        oled_write_cmd(0xB0 + page); // 设置页地址
        oled_write_cmd(0x00);        // 设置列地址低4位
        oled_write_cmd(0x10);        // 设置列地址高4位
        
        for (uint16_t col = 0; col < OLED_WIDTH; col++) {
            oled_write_data(g_oled_driver->gram[col * OLED_PAGE_NUM + page]);
        }
    }
}

// 快速刷新显示（优化版本）
void oled_driver_refresh_fast(void)
{
    if (g_oled_driver == NULL || !g_oled_driver->initialized) {
        return;
    }
    
    // 使用原始刷新方法，确保所有页面都被正确刷新
    for (uint8_t page = 0; page < OLED_PAGE_NUM; page++) {
        oled_write_cmd(0xB0 + page); // 设置页地址
        oled_write_cmd(0x00);        // 设置列地址低4位
        oled_write_cmd(0x10);        // 设置列地址高4位
        
        for (uint16_t col = 0; col < OLED_WIDTH; col++) {
            oled_write_data(g_oled_driver->gram[col * OLED_PAGE_NUM + page]);
        }
    }
}

// 部分刷新（只刷新脏页面）
void oled_driver_refresh_partial(void)
{
    if (g_oled_driver == NULL || !g_oled_driver->initialized) {
        return;
    }
    
    for (uint8_t page = 0; page < OLED_PAGE_NUM; page++) {
        if (g_oled_driver->dirty_region[page]) {
            oled_write_cmd(0xB0 + page); // 设置页地址
            oled_write_cmd(0x00);        // 设置列地址低4位
            oled_write_cmd(0x10);        // 设置列地址高4位
            
            for (uint16_t col = 0; col < OLED_WIDTH; col++) {
                oled_write_data(g_oled_driver->gram[col * OLED_PAGE_NUM + page]);
            }
            g_oled_driver->dirty_region[page] = false; // 清除脏标记
        }
    }
}

// 标记页面为脏
static void oled_driver_mark_dirty(uint8_t page)
{
    if (g_oled_driver == NULL || !g_oled_driver->initialized || page >= OLED_PAGE_NUM) {
        return;
    }
    
    g_oled_driver->dirty_region[page] = true;
}

// 画点（前缓冲区）
void oled_driver_draw_pixel(uint16_t x, uint16_t y, bool set)
{
    if (g_oled_driver == NULL || !g_oled_driver->initialized || x >= OLED_WIDTH || y >= OLED_HEIGHT) {
        return;
    }
    
    uint8_t page = y / 8;
    uint8_t bit = y % 8;
    
    if (set) {
        g_oled_driver->gram[x * OLED_PAGE_NUM + page] |= (1 << bit);
    } else {
        g_oled_driver->gram[x * OLED_PAGE_NUM + page] &= ~(1 << bit);
    }
    
    // 标记该页面为脏
    oled_driver_mark_dirty(page);
}

// 画点（后缓冲区）
void oled_driver_draw_pixel_backbuffer(uint16_t x, uint16_t y, bool set)
{
    if (g_oled_driver == NULL || !g_oled_driver->initialized || 
        g_oled_driver->back_buffer == NULL || x >= OLED_WIDTH || y >= OLED_HEIGHT) {
        return;
    }
    
    uint8_t page = y / 8;
    uint8_t bit = y % 8;
    
    if (set) {
        g_oled_driver->back_buffer[x * OLED_PAGE_NUM + page] |= (1 << bit);
    } else {
        g_oled_driver->back_buffer[x * OLED_PAGE_NUM + page] &= ~(1 << bit);
    }
}

// 交换缓冲区
void oled_driver_swap_buffers(void)
{
    if (g_oled_driver == NULL || !g_oled_driver->initialized || g_oled_driver->back_buffer == NULL) {
        return;
    }
    
    // 将后缓冲区数据复制到前缓冲区
    memcpy(g_oled_driver->gram, g_oled_driver->back_buffer, OLED_WIDTH * OLED_PAGE_NUM);
    
    // 标记所有页面为脏
    for (uint8_t page = 0; page < OLED_PAGE_NUM; page++) {
        g_oled_driver->dirty_region[page] = true;
    }
}

// 设置显示开关
void oled_driver_set_display(bool on)
{
    if (g_oled_driver == NULL || !g_oled_driver->initialized) {
        return;
    }
    
    oled_write_cmd(on ? 0xAF : 0xAE);
}

// 设置显示模式
void oled_driver_set_display_mode(oled_display_mode_t mode)
{
    if (g_oled_driver == NULL || !g_oled_driver->initialized) {
        return;
    }
    
    oled_write_cmd(mode == OLED_DISPLAY_INVERTED ? 0xA7 : 0xA6);
    g_oled_driver->display_mode = mode;
}

// 获取驱动状态
bool oled_driver_is_initialized(void)
{
    return g_oled_driver != NULL && g_oled_driver->initialized;
}

uint16_t oled_driver_get_width(void)
{
    return OLED_WIDTH;
}

uint16_t oled_driver_get_height(void)
{
    return OLED_HEIGHT;
}