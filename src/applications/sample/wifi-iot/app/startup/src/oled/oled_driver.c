#include "oled_driver.h"
#include "hi_i2c.h"
#include "hi_io.h"
#include "hi_gpio.h"
#include <stdio.h>
#include <string.h>

static oled_driver_t g_oled_driver = {0};

// I2C写字节函数
static int i2c_write_byte(uint8_t data)
{
    hi_i2c_data i2c_data = {
        .send_buf = &data,
        .send_len = 1,
        .receive_buf = NULL,
        .receive_len = 0
    };
    
    return hi_i2c_writedev(OLED_I2C_IDX, OLED_ADDRESS, &i2c_data);
}

// 写命令到OLED
static void oled_write_cmd(uint8_t cmd)
{
    uint8_t buf[2] = {OLED_CMD, cmd};
    hi_i2c_data i2c_data = {
        .send_buf = buf,
        .send_len = 2,
        .receive_buf = NULL,
        .receive_len = 0
    };
    hi_i2c_writedev(OLED_I2C_IDX, OLED_ADDRESS, &i2c_data);
}

// 写数据到OLED
static void oled_write_data(uint8_t data)
{
    uint8_t buf[2] = {OLED_DATA, data};
    hi_i2c_data i2c_data = {
        .send_buf = buf,
        .send_len = 2,
        .receive_buf = NULL,
        .receive_len = 0
    };
    hi_i2c_writedev(OLED_I2C_IDX, OLED_ADDRESS, &i2c_data);
}

// OLED初始化
int oled_driver_init(void)
{
    // 初始化I2C引脚
    hi_io_set_func(HI_IO_NAME_GPIO_9, OLED_I2C_SCL_PIN);
    hi_io_set_func(HI_IO_NAME_GPIO_10, OLED_I2C_SDA_PIN);
    
    // 初始化I2C控制器
    hi_i2c_init(OLED_I2C_IDX, 400000); // 400kHz
    
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
    
    g_oled_driver.initialized = true;
    g_oled_driver.display_mode = OLED_DISPLAY_NORMAL;
    
    printf("OLED Driver Initialized Success!\n");
    return 0;
}

void oled_driver_deinit(void)
{
    oled_write_cmd(0xAE); // 关闭显示
    g_oled_driver.initialized = false;
}

// 清屏
void oled_driver_clear(void)
{
    memset(g_oled_driver.gram, 0, sizeof(g_oled_driver.gram));
}

// 刷新显示
void oled_driver_refresh(void)
{
    for (uint8_t page = 0; page < OLED_PAGE_NUM; page++) {
        oled_write_cmd(0xB0 + page); // 设置页地址
        oled_write_cmd(0x00);        // 设置列地址低4位
        oled_write_cmd(0x10);        // 设置列地址高4位
        
        for (uint16_t col = 0; col < OLED_WIDTH; col++) {
            oled_write_data(g_oled_driver.gram[col][page]);
        }
    }
}

// 设置显示开关
void oled_driver_set_display(bool on)
{
    oled_write_cmd(on ? 0xAF : 0xAE);
}

// 设置显示模式
void oled_driver_set_display_mode(oled_display_mode_t mode)
{
    oled_write_cmd(mode == OLED_DISPLAY_INVERTED ? 0xA7 : 0xA6);
    g_oled_driver.display_mode = mode;
}

// 画点
void oled_driver_draw_pixel(uint16_t x, uint16_t y, bool set)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    
    uint8_t page = y / 8;
    uint8_t bit = y % 8;
    
    if (set) {
        g_oled_driver.gram[x][page] |= (1 << bit);
    } else {
        g_oled_driver.gram[x][page] &= ~(1 << bit);
    }
}

// 获取驱动状态
bool oled_driver_is_initialized(void)
{
    return g_oled_driver.initialized;
}

uint16_t oled_driver_get_width(void)
{
    return OLED_WIDTH;
}

uint16_t oled_driver_get_height(void)
{
    return OLED_HEIGHT;
}