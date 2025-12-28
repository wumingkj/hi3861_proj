# OLED显示模块

## 模块概述

OLED显示模块提供基于SSD1306驱动的OLED屏幕控制功能，支持文本、数字和温湿度信息的显示。该模块采用I2C通信协议，提供简洁易用的API接口，适用于设备状态信息的可视化展示。

## 功能特性

- ✅ **SSD1306驱动支持** - 标准0.96寸OLED屏幕
- ✅ **I2C通信接口** - 高速稳定的数据传输
- ✅ **文本显示** - 支持不同尺寸的文本显示
- ✅ **数字显示** - 格式化数字显示功能
- ✅ **温湿度显示** - 专门的温湿度信息显示
- ✅ **屏幕控制** - 清屏、刷新等基本操作
- ✅ **状态检测** - 设备就绪状态检测

## 硬件配置

### 屏幕规格
- **驱动芯片**: SSD1306
- **分辨率**: 128x64像素
- **通信接口**: I2C
- **尺寸**: 0.96英寸

### GPIO引脚分配
```c
// OLED模块使用的GPIO引脚配置
#define OLED_I2C_SCL_PIN    HI_IO_NAME_GPIO_3  // I2C时钟线
#define OLED_I2C_SDA_PIN    HI_IO_NAME_GPIO_4  // I2C数据线
```

## API接口

### 初始化与状态管理
```c
/**
 * @brief OLED模块初始化
 * @return 成功返回0，失败返回错误码
 */
int OLED_Init(void);

/**
 * @brief OLED模块反初始化
 */
void OLED_Deinit(void);

/**
 * @brief 检查OLED是否就绪
 * @return true表示就绪，false表示未就绪
 */
bool OLED_IsReady(void);
```

### 显示控制函数
```c
/**
 * @brief 清空OLED屏幕
 */
void OLED_Clear(void);

/**
 * @brief 刷新OLED显示（将缓存数据写入屏幕）
 */
void OLED_Refresh(void);
```

### 内容显示函数
```c
/**
 * @brief 在指定位置显示字符串
 * @param x 横坐标（0-127）
 * @param y 纵坐标（0-7，每行8像素）
 * @param str 要显示的字符串
 * @param size 字体大小（1-3）
 */
void OLED_ShowString(uint16_t x, uint16_t y, const char* str, uint8_t size);

/**
 * @brief 在指定位置显示数字
 * @param x 横坐标（0-127）
 * @param y 纵坐标（0-7）
 * @param num 要显示的数字
 * @param len 数字长度
 * @param size 字体大小（1-3）
 */
void OLED_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size);

/**
 * @brief 显示温湿度信息（专用格式）
 * @param temperature 温度值（摄氏度）
 * @param humidity 湿度值（百分比）
 */
void OLED_ShowTemperatureHumidity(float temperature, int humidity);
```

## 使用示例

### 基本初始化与显示
```c
#include "oled.h"

// OLED模块初始化
if (OLED_Init() != 0) {
    printf("OLED初始化失败\n");
    return;
}

// 检查OLED状态
if (!OLED_IsReady()) {
    printf("OLED未就绪\n");
    return;
}

// 清屏并显示信息
OLED_Clear();
OLED_ShowString(0, 0, "Hi3861 IoT设备", 2);
OLED_ShowString(0, 2, "状态: 运行中", 1);
OLED_Refresh();
```

### 温湿度监控显示
```c
// 定期更新温湿度显示
void update_display(float temp, int hum) {
    OLED_Clear();
    OLED_ShowString(0, 0, "环境监测", 2);
    OLED_ShowTemperatureHumidity(temp, hum);
    OLED_ShowString(0, 4, "WiFi: 已连接", 1);
    OLED_ShowString(0, 5, "NFC: 就绪", 1);
    OLED_Refresh();
}

// 在主循环中调用
while (1) {
    float temperature = read_temperature();
    int humidity = read_humidity();
    update_display(temperature, humidity);
    osDelay(5000); // 5秒更新一次
}
```

### 多行信息显示
```c
// 显示设备状态信息
void show_device_status(void) {
    OLED_Clear();
    
    // 第一行：设备名称
    OLED_ShowString(0, 0, "Hi3861 WiFi IoT", 1);
    
    // 第二行：网络状态
    OLED_ShowString(0, 1, "网络: ", 1);
    if (wifi_connected()) {
        OLED_ShowString(40, 1, "已连接", 1);
    } else {
        OLED_ShowString(40, 1, "未连接", 1);
    }
    
    // 第三行：IP地址
    char ip_str[16];
    get_ip_address(ip_str);
    OLED_ShowString(0, 2, "IP: ", 1);
    OLED_ShowString(24, 2, ip_str, 1);
    
    // 第四行：运行时间
    OLED_ShowString(0, 3, "运行: ", 1);
    OLED_ShowNum(36, 3, get_uptime(), 6, 1);
    OLED_ShowString(84, 3, "秒", 1);
    
    OLED_Refresh();
}
```

## 内部实现

### I2C通信协议
```c
// I2C写操作
bool oled_i2c_write(uint8_t reg, uint8_t data);
bool oled_i2c_write_buffer(uint8_t reg, uint8_t *data, uint16_t len);

// I2C读操作
bool oled_i2c_read(uint8_t reg, uint8_t *data, uint16_t len);
```

### 显示缓存管理
模块维护一个128x64的显示缓存：
```c
// 显示缓存（128x64位，每字节8像素）
uint8_t oled_buffer[128][8];

// 缓存刷新机制
void oled_refresh_buffer(void) {
    // 将缓存数据通过I2C写入OLED
    for (int page = 0; page < 8; page++) {
        oled_set_page_address(page);
        oled_i2c_write_buffer(0x40, oled_buffer[0][page], 128);
    }
}
```

### 字体系统
支持多种字体尺寸：
```c
// 字体定义
extern const uint8_t font_6x8[][6];   // 6x8小字体
extern const uint8_t font_8x16[][16]; // 8x16中字体  
extern const uint8_t font_12x24[][36]; // 12x24大字体
```

## 性能优化

### 显示优化
- **局部刷新**: 只更新变化的部分
- **缓存机制**: 减少I2C通信次数
- **批量写入**: 一次传输多字节数据

### 内存优化
- **静态字体**: 字体数据存储在Flash中
- **紧凑缓存**: 优化的显示缓存结构
- **无动态分配**: 避免内存碎片

## 集成示例

### 与传感器模块集成
```c
// 传感器数据显示
void show_sensor_data(void) {
    sensor_data_t data = read_sensors();
    
    OLED_Clear();
    OLED_ShowString(0, 0, "传感器数据", 2);
    OLED_ShowString(0, 2, "温度: ", 1);
    OLED_ShowNum(36, 2, (int)data.temperature, 2, 1);
    OLED_ShowString(52, 2, "C", 1);
    
    OLED_ShowString(0, 3, "湿度: ", 1);
    OLED_ShowNum(36, 3, data.humidity, 2, 1);
    OLED_ShowString(52, 3, "%", 1);
    
    OLED_ShowString(0, 4, "光照: ", 1);
    OLED_ShowNum(36, 4, data.light, 3, 1);
    OLED_ShowString(60, 4, "lux", 1);
    
    OLED_Refresh();
}
```

### 与网络模块集成
```c
// 网络状态显示
void show_network_status(void) {
    OLED_Clear();
    
    if (wifi_is_connected()) {
        OLED_ShowString(0, 0, "WiFi状态", 2);
        OLED_ShowString(0, 2, "SSID: ", 1);
        OLED_ShowString(36, 2, get_ssid(), 1);
        
        OLED_ShowString(0, 3, "IP: ", 1);
        OLED_ShowString(24, 3, get_ip_address_str(), 1);
        
        OLED_ShowString(0, 4, "信号: ", 1);
        OLED_ShowNum(36, 4, get_rssi(), 2, 1);
        OLED_ShowString(52, 4, "dBm", 1);
    } else {
        OLED_ShowString(0, 0, "网络状态", 2);
        OLED_ShowString(0, 2, "WiFi未连接", 1);
        OLED_ShowString(0, 3, "正在扫描...", 1);
    }
    
    OLED_Refresh();
}
```

## 故障排除

### 常见问题
1. **初始化失败**: 检查I2C线路连接和电源
2. **显示异常**: 验证屏幕型号和通信协议
3. **内容不更新**: 确保调用OLED_Refresh()

### 调试信息
```c
// 启用调试输出
#define OLED_DEBUG 1

#if OLED_DEBUG
#define oled_printf(...) printf(__VA_ARGS__)
#else
#define oled_printf(...)
#endif
```

## 相关文件

- `oled.h` - OLED模块头文件，包含API声明
- `oled.c` - OLED模块实现文件
- `fonts/` - 字体文件目录（如果存在）

## 扩展功能

### 图形绘制（可选）
```c
// 可扩展的图形功能
void OLED_DrawPixel(uint16_t x, uint16_t y);
void OLED_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void OLED_DrawRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
```

### 动画效果（可选）
```c
// 简单的动画支持
void OLED_ShowLoadingAnimation(void);
void OLED_ShowProgressBar(uint8_t progress);
```

[返回主文档](../../../../../../../../README.md)