#include "oled_driver.h"
#include "hi_io.h"
#include "hi_gpio.h"
#include "hi_time.h"
#include "hi_types_base.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 全局驱动实例 - 改为指针
static oled_driver_t* g_oled_driver = NULL;

// OLED I2C 地址（7位地址 0x3C 左移 1 位）
#define OLED_I2C_ADDR           0x78

// ======================== 超精简软件 I2C（直接寄存器操作） ========================
// GPIO 寄存器（见 hi3861_platform_base.h + hi_gpio.h）
#define GPIO_REG_DR             (0x50006000 + 0x00)  // Data output
#define GPIO_REG_DDR            (0x50006000 + 0x04)  // Direction
#define SCL_BIT                 ((uint16_t)(1 << 9))
#define SDA_BIT                 ((uint16_t)(1 << 10))

// 影子寄存器 — 初始化时从硬件读入，避免 read-modify-write
static uint16_t g_gpio_shadow = 0;

static void sw_i2c_shadow_init(void)
{
    // 设置 SCL/SDA 初始输出为高电平（I2C 总线空闲），不碰其他 GPIO
    uint16_t dr_val;
    hi_reg_read16(GPIO_REG_DR, dr_val);
    dr_val = (dr_val & ~(SCL_BIT | SDA_BIT)) | SCL_BIT | SDA_BIT;
    hi_reg_write16(GPIO_REG_DR, dr_val);
    g_gpio_shadow = dr_val;
}

// 起始信号：SDA 1→0, SCL=1
static void sw_i2c_start(void)
{
    uint16_t ddr;
    hi_reg_read16(GPIO_REG_DDR, ddr);
    hi_reg_write16(GPIO_REG_DDR, ddr | SCL_BIT | SDA_BIT);

    // ★ 关键修复：重新读取寄存器，确保 shadow 反映所有 GPIO 的当前状态
    //   避免 stale shadow 覆写其他外设（如 UART 下载用的 GPIO0/1）
    hi_reg_read16(GPIO_REG_DR, g_gpio_shadow);

    uint16_t s = g_gpio_shadow;
    s |= SCL_BIT | SDA_BIT;               // SCL=1, SDA=1
    hi_reg_write16(GPIO_REG_DR, s);
    s &= ~SDA_BIT;                         // SDA=0 (SCL=1)
    hi_reg_write16(GPIO_REG_DR, s);
    s &= ~SCL_BIT;                         // SCL=0 (SDA=0)
    hi_reg_write16(GPIO_REG_DR, s);
    g_gpio_shadow = s;
}

// 停止信号：SDA 0→1, SCL=1
static void sw_i2c_stop(void)
{
    uint16_t s = g_gpio_shadow;
    s &= ~SCL_BIT;                         // SCL=0
    s &= ~SDA_BIT;                         // SDA=0
    hi_reg_write16(GPIO_REG_DR, s);
    s |= SCL_BIT;                          // SCL=1 (SDA=0)
    hi_reg_write16(GPIO_REG_DR, s);
    s |= SDA_BIT;                          // SDA=1 (SCL=1)
    hi_reg_write16(GPIO_REG_DR, s);
    g_gpio_shadow = s;
}

// 写 1 字节（8 数据位 + 1 ACK），SDA 先建立再产生 SCL 脉冲
static void sw_i2c_write_byte(uint8_t data)
{
    uint16_t s = g_gpio_shadow;

    for (int i = 0; i < 8; i++) {
        // 1) SDA 建立（SCL=0）
        if (data & 0x80) {
            s |= SDA_BIT;
        } else {
            s &= ~SDA_BIT;
        }
        hi_reg_write16(GPIO_REG_DR, s);           // SDA 稳定，SCL=0
        // 2) SCL 上升沿（采样数据）
        hi_reg_write16(GPIO_REG_DR, s | SCL_BIT); // SCL=1
        // 3) SCL 下降沿
        hi_reg_write16(GPIO_REG_DR, s);           // SCL=0
        data <<= 1;
    }

    // ACK：主控拉低 SDA + 第 9 个 SCL 脉冲（忽略从机应答）
    s &= ~SDA_BIT;
    hi_reg_write16(GPIO_REG_DR, s);               // SDA=0, SCL=0
    hi_reg_write16(GPIO_REG_DR, s | SCL_BIT);     // SCL=1
    hi_reg_write16(GPIO_REG_DR, s);               // SCL=0

    g_gpio_shadow = s;                            // SCL=0, SDA=0
}

// 软件 I2C 批量发送：START + 地址(写) + 数据[0..n-1] + STOP
static void oled_i2c_sw_write(const uint8_t* data, uint16_t len)
{
    sw_i2c_start();
    sw_i2c_write_byte(OLED_I2C_ADDR & 0xFE);
    for (uint16_t i = 0; i < len; i++) {
        sw_i2c_write_byte(data[i]);
    }
    sw_i2c_stop();
}

// ======================== OLED I2C 高层接口 ========================

// I2C 发送缓冲区：控制字节(1) + 最大 GRAM 数据(1024)
#define OLED_I2C_TX_BUF_SIZE    (OLED_WIDTH * OLED_PAGE_NUM + 1)
static uint8_t g_i2c_tx_buf[OLED_I2C_TX_BUF_SIZE];

// 单事务刷新缓冲区：6条命令×2字节(控制+命令) + 1控制字节 + 1024数据 = 1037
#define OLED_REFRESH_BUF_SIZE   (6 * 2 + 1 + OLED_WIDTH * OLED_PAGE_NUM)
static uint8_t g_refresh_buf[OLED_REFRESH_BUF_SIZE];

static void oled_i2c_tx(uint8_t ctrl_byte, const uint8_t* data, uint16_t len)
{
    if (data == NULL && len > 0) {
        return;
    }

    g_i2c_tx_buf[0] = ctrl_byte;
    if (len > 0) {
        if (len > OLED_I2C_TX_BUF_SIZE - 1) {
            return;
        }
        memcpy(g_i2c_tx_buf + 1, data, len);
    }

    oled_i2c_sw_write(g_i2c_tx_buf, len + 1);
}

// 写单条命令
static void oled_write_cmd(uint8_t cmd)
{
    oled_i2c_tx(OLED_CMD, &cmd, 1);
}

// 写单个数据
static void oled_write_data(uint8_t data)
{
    oled_i2c_tx(OLED_DATA, &data, 1);
}

// 批量写命令
static void oled_write_cmd_burst(const uint8_t* cmds, uint16_t len)
{
    oled_i2c_tx(OLED_CMD, cmds, len);
}

// 批量写数据（支持全屏 1024 字节）
static void oled_write_data_burst(const uint8_t* data, uint16_t len)
{
    oled_i2c_tx(OLED_DATA, data, len);
}

// OLED初始化 - SDK硬件I2C版本（400kHz）
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
    
    // 1. 初始化 GPIO 子系统
    hi_gpio_init();

    // 2. 设置引脚为 GPIO 模式（软件 I2C），上拉使能
    hi_io_set_func(OLED_SW_I2C_SCL_PIN, HI_IO_FUNC_GPIO_9_GPIO);
    hi_io_set_pull(OLED_SW_I2C_SCL_PIN, HI_IO_PULL_UP);
    hi_io_set_func(OLED_SW_I2C_SDA_PIN, HI_IO_FUNC_GPIO_10_GPIO);
    hi_io_set_pull(OLED_SW_I2C_SDA_PIN, HI_IO_PULL_UP);

    // 初始化 GPIO 影子寄存器（软件 I2C 用）
    sw_i2c_shadow_init();

    // 3. 等待 SSD1306 上电稳定
    hi_udelay(100 * 1000);

    // 4. SSD1306 初始化序列
    oled_write_cmd(0xAE); // 关闭显示
    oled_write_cmd(0xD5); // 设置时钟分频因子/振荡器频率
    oled_write_cmd(0x80); // 分频因子=1, 振荡器频率=default
    oled_write_cmd(0xA8); // 设置驱动路数（多路复用率）
    oled_write_cmd(0x3F); // 1/64 duty (64行)
    oled_write_cmd(0xD3); // 设置显示偏移
    oled_write_cmd(0x00); // 无偏移
    oled_write_cmd(0x40); // 设置显示起始行 = 0
    oled_write_cmd(0x8D); // 电荷泵设置
    oled_write_cmd(0x14); // 启用电荷泵
    oled_write_cmd(0x20); // 设置内存地址模式
    oled_write_cmd(0x00); // 水平地址模式（支持0x21/0x22范围命令，单事务刷新）
    oled_write_cmd(0xA1); // 段重映射: 0->127
    oled_write_cmd(0xC8); // COM扫描方向: 重定义模式（配合正序GRAM: page=y/8, bit=1<<(y%8)）
    oled_write_cmd(0xDA); // 设置COM硬件引脚配置
    oled_write_cmd(0x12); // Sequential COM pin config
    oled_write_cmd(0x81); // 设置对比度
    oled_write_cmd(0xEF); // 对比度值（普中demo用0xEF，比0xFF稍低但更稳定）
    oled_write_cmd(0xD9); // 设置预充电周期
    oled_write_cmd(0xF1); // PHASE1=1, PHASE2=15（普中demo用0xF1）
    oled_write_cmd(0xDB); // 设置VCOMH电压倍率
    oled_write_cmd(0x30); // 0.83xVcc（普中demo用0x30）
    oled_write_cmd(0xA4); // 输出跟随RAM内容
    oled_write_cmd(0xA6); // 正常显示（非反色）
    oled_write_cmd(0xAF); // 开启显示
    
    // 清屏
    oled_driver_clear();
    oled_driver_refresh();
    
    g_oled_driver->saved_gram = NULL;
    g_oled_driver->initialized = true;
    g_oled_driver->display_mode = OLED_DISPLAY_NORMAL;
    
    printf("OLED Driver Init OK (SW I2C), mem=%u bytes\n",
           (unsigned)(sizeof(oled_driver_t) + (OLED_WIDTH * OLED_PAGE_NUM * 2) + OLED_PAGE_NUM));
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

// ======================== 单事务I2C刷新（Hisilicon ssd1306.c 方式，极速） ========================
// SSD1306 I2C协议：控制字节格式 [Co, D/C#, 0,0,0,0,0,0]
//   Co=1: 后续还有控制字节; Co=0: 后续全是数据
//   D/C#=0: 命令; D/C#=1: 显示数据
// 组合: [0x80,cmd0, 0x80,cmd1, ..., 0x40, data0, data1, ..., data1023] → 单次I2C事务

void oled_driver_refresh(void)
{
    if (g_oled_driver == NULL || !g_oled_driver->initialized) {
        return;
    }

    uint32_t count = 0;

    static const uint8_t set_range_cmds[] = {
        0x21, 0x00, 0x7F,
        0x22, 0x00, 0x07,
    };

    for (uint32_t i = 0; i < sizeof(set_range_cmds); i++) {
        g_refresh_buf[count++] = 0x80;
        g_refresh_buf[count++] = set_range_cmds[i];
    }

    g_refresh_buf[count++] = 0x40;
    memcpy(&g_refresh_buf[count], g_oled_driver->gram, OLED_WIDTH * OLED_PAGE_NUM);
    count += OLED_WIDTH * OLED_PAGE_NUM;

    // 软件 I2C 单次事务写入
    oled_i2c_sw_write(g_refresh_buf, count);
}

// 快速刷新显示
void oled_driver_refresh_fast(void)
{
    oled_driver_refresh();
}

// 部分刷新（脏页模式）——水平地址模式下只刷脏页
void oled_driver_refresh_partial(void)
{
    if (g_oled_driver == NULL || !g_oled_driver->initialized) {
        return;
    }

    for (uint8_t page = 0; page < OLED_PAGE_NUM; page++) {
        if (g_oled_driver->dirty_region[page]) {
            // 设置列范围和单页范围
            uint8_t cmd[] = {0x21, 0x00, 0x7F, 0x22, page, page};
            oled_write_cmd_burst(cmd, sizeof(cmd));
            oled_write_data_burst(&g_oled_driver->gram[page * OLED_WIDTH], OLED_WIDTH);
            g_oled_driver->dirty_region[page] = false;
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

// 直接写入显存块（页格式数据）
// data 排列: data[page * w + col], 每字节 = 8 个垂直像素
void oled_driver_write_gram_block(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data)
{
    if (g_oled_driver == NULL || !g_oled_driver->initialized || data == NULL) {
        return;
    }
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT || w == 0 || h == 0) {
        return;
    }
    
    // 裁剪到屏幕范围
    uint16_t end_x = x + w;
    uint16_t end_y = y + h;
    if (end_x > OLED_WIDTH)  end_x = OLED_WIDTH;
    if (end_y > OLED_HEIGHT) end_y = OLED_HEIGHT;
    w = end_x - x;
    h = end_y - y;
    
    uint16_t pages = (h + 7) / 8;
    uint8_t start_page = y / 8;
    uint8_t bit_offset = y % 8;  // 起始行在页内的偏移
    
    for (uint16_t p = 0; p < pages; p++) {
        uint8_t sp = start_page + p;
        if (sp >= OLED_PAGE_NUM) break;
        
        for (uint16_t col = 0; col < w; col++) {
            uint16_t sc = x + col;
            
            uint8_t src = data[p * w + col];
            
            if (bit_offset == 0) {
                // 页对齐：直接复制
                g_oled_driver->gram[sp * OLED_WIDTH + sc] = src;
            } else {
                // 非页对齐：需要和相邻页合并
                // 当前页取 src 的高 (8 - bit_offset) 位
                // 下一页取 src 的低 bit_offset 位
                uint8_t shift = 8 - bit_offset;
                g_oled_driver->gram[sp * OLED_WIDTH + sc] = 
                    (g_oled_driver->gram[sp * OLED_WIDTH + sc] & ~(0xFF << bit_offset)) |
                    ((src << bit_offset) & 0xFF);
                
                if (sp + 1 < OLED_PAGE_NUM) {
                    g_oled_driver->gram[(sp + 1) * OLED_WIDTH + sc] =
                        (g_oled_driver->gram[(sp + 1) * OLED_WIDTH + sc] & (0xFF << bit_offset)) |
                        (src >> shift);
                }
            }
        }
        
        oled_driver_mark_dirty(sp);
    }
}

// ======================== 行优先 bitmap → 页优先格式转换 ========================
// 将标准行优先 bitmap（row-major, MSB左）转换为页优先格式（page-major）
// 输出 page_buf 排列: page_buf[page * w + col], 每字节 = 8 个垂直像素
static void oled_driver_bitmap_to_page(const uint8_t *bitmap, uint16_t w, uint16_t h, uint8_t *page_buf)
{
    uint16_t w_bytes = (w + 7) / 8;
    uint16_t pages = (h + 7) / 8;
    
    for (uint16_t p = 0; p < pages; p++) {
        for (uint16_t col = 0; col < w; col++) {
            uint8_t byte = 0;
            
            // 将 bitmap 中对应此列 8 行的像素打包成一个字节
            for (uint8_t bit = 0; bit < 8; bit++) {
                uint16_t row = p * 8 + bit;
                if (row >= h) break;
                
                uint8_t b = bitmap[row * w_bytes + col / 8];
                if (b & (1 << (7 - (col % 8)))) {
                    byte |= (1 << bit);
                }
            }
            
            page_buf[p * w + col] = byte;
        }
    }
}

// 从水平 bitmap（行优先格式）写入显存
// bitmap 排列: bitmap[row * ((w+7)/8) + col/8], 每字节 = 8 个水平像素（MSB 左）
void oled_driver_write_gram_block_from_bitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *bitmap)
{
    if (g_oled_driver == NULL || !g_oled_driver->initialized || bitmap == NULL) {
        return;
    }
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT || w == 0 || h == 0) {
        return;
    }
    
    // 裁剪到屏幕范围
    uint16_t end_x = x + w;
    uint16_t end_y = y + h;
    if (end_x > OLED_WIDTH)  end_x = OLED_WIDTH;
    if (end_y > OLED_HEIGHT) end_y = OLED_HEIGHT;
    w = end_x - x;
    h = end_y - y;
    
    // 临时页缓冲区，最大 128*8=1024 字节
    // 静态缓冲区避免栈溢出，渲染串行化保证安全
    static uint8_t s_page_buf[OLED_WIDTH * OLED_PAGE_NUM];
    
    // 行优先 → 页优先转换
    oled_driver_bitmap_to_page(bitmap, w, h, s_page_buf);
    
    // 复用 write_gram_block 写入 gram（处理裁剪/偏移/脏标记）
    oled_driver_write_gram_block(x, y, w, h, s_page_buf);
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
        g_oled_driver->gram[page * OLED_WIDTH + x] |= (1 << bit);
    } else {
        g_oled_driver->gram[page * OLED_WIDTH + x] &= ~(1 << bit);
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
        g_oled_driver->back_buffer[page * OLED_WIDTH + x] |= (1 << bit);
    } else {
        g_oled_driver->back_buffer[page * OLED_WIDTH + x] &= ~(1 << bit);
    }
}

// 交换缓冲区（O(1) 指针交换）
void oled_driver_swap_buffers(void)
{
    if (g_oled_driver == NULL || !g_oled_driver->initialized || g_oled_driver->back_buffer == NULL) {
        return;
    }
    
    // 交换 gram ↔ back_buffer 指针
    uint8_t *tmp = g_oled_driver->gram;
    g_oled_driver->gram = g_oled_driver->back_buffer;
    g_oled_driver->back_buffer = tmp;
    
    // 清空新的后缓冲区（旧前缓冲区内容，已不需要）
    memset(g_oled_driver->back_buffer, 0, OLED_WIDTH * OLED_PAGE_NUM);
    
    // 标记所有页面为脏
    for (uint8_t page = 0; page < OLED_PAGE_NUM; page++) {
        g_oled_driver->dirty_region[page] = true;
    }
}

// 开始双缓冲渲染：将 gram 重定向到后缓冲区并清空，所有绘制操作写入不可见区域
void oled_driver_begin_render(void)
{
    if (g_oled_driver == NULL || !g_oled_driver->initialized || g_oled_driver->back_buffer == NULL) {
        return;
    }
    
    // 嵌套保护：如果 saved_gram != NULL，说明外层已调过 begin_render，跳过
    if (g_oled_driver->saved_gram != NULL) {
        return;
    }
    
    g_oled_driver->saved_gram = g_oled_driver->gram;
    g_oled_driver->gram = g_oled_driver->back_buffer;
    memset(g_oled_driver->gram, 0, OLED_WIDTH * OLED_PAGE_NUM);
}

// 开始双缓冲渲染（保留内容模式）：将 gram 重定向到后缓冲区，同时把前缓冲区内容复制过来
// 用于需要叠加绘制（如 DrawText 叠在已有图片上）的场景
void oled_driver_begin_render_copy(void)
{
    if (g_oled_driver == NULL || !g_oled_driver->initialized || g_oled_driver->back_buffer == NULL) {
        return;
    }
    
    // 嵌套保护：如果 saved_gram != NULL，说明外层已调过 begin_render，跳过
    if (g_oled_driver->saved_gram != NULL) {
        return;
    }
    
    g_oled_driver->saved_gram = g_oled_driver->gram;
    g_oled_driver->gram = g_oled_driver->back_buffer;
    memcpy(g_oled_driver->gram, g_oled_driver->saved_gram, OLED_WIDTH * OLED_PAGE_NUM);
}

// 结束双缓冲渲染：恢复 gram 指针，交换缓冲区，刷新到屏幕
void oled_driver_end_render(void)
{
    if (g_oled_driver == NULL || !g_oled_driver->initialized || g_oled_driver->back_buffer == NULL) {
        return;
    }
    
    // 嵌套保护：如果 saved_gram == NULL，说明没有活跃的渲染会话，跳过
    if (g_oled_driver->saved_gram == NULL) {
        return;
    }
    
    g_oled_driver->gram = g_oled_driver->saved_gram;
    g_oled_driver->saved_gram = NULL;  // 清空标记，允许下一轮 begin_render
    oled_driver_swap_buffers();
    oled_driver_refresh_fast();
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

// 设置对比度（0~255，越大越亮）
void oled_driver_set_contrast(uint8_t contrast)
{
    if (g_oled_driver == NULL || !g_oled_driver->initialized) {
        return;
    }
    
    oled_write_cmd(0x81);
    oled_write_cmd(contrast);
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