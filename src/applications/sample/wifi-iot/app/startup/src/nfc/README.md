# NFC模块

## 模块概述

NFC模块基于NT3H芯片实现近场通信功能，支持NFC标签的读写操作，主要用于WiFi配置信息的快速传输。该模块实现了NDEF协议，支持URI和文本记录的存储与读取。

## 功能特性

- ✅ **NT3H芯片支持** - 完整的NFC读写功能
- ✅ **NDEF协议实现** - 标准的NFC数据交换格式
- ✅ **URI记录支持** - 存储和读取网址链接
- ✅ **文本记录支持** - 存储和读取文本内容
- ✅ **WiFi配置传输** - 快速传输WiFi网络配置
- ✅ **多记录存储** - 支持多个记录位置存储
- ✅ **数据清除** - 安全的NDEF数据清除功能

## 硬件配置

### 芯片型号
- **NFC芯片**: NT3H系列
- **通信接口**: I2C
- **工作频率**: 13.56MHz

### GPIO引脚分配
```c
// NFC模块使用的GPIO引脚配置
#define NFC_I2C_SCL_PIN    HI_IO_NAME_GPIO_0  // I2C时钟线
#define NFC_I2C_SDA_PIN    HI_IO_NAME_GPIO_1  // I2C数据线
#define NFC_IRQ_PIN        HI_IO_NAME_GPIO_2  // 中断引脚
```

## API接口

### 初始化函数
```c
/**
 * @brief NFC模块初始化
 * @return 成功返回HI_ERR_SUCCESS，失败返回错误码
 */
uint32_t nfc_init(void);
```

### 数据存储函数
```c
/**
 * @brief 写入URI记录到NT3H指定位置
 * @param position 存储的位置
 * @param http 存储的网址链接
 * @return 成功返回true，失败返回false
 */
bool storeUrihttp(RecordPosEnu position, uint8_t *http);

/**
 * @brief 写入文本记录到NT3H指定位置
 * @param position 存储的位置
 * @param text 存储的文本内容
 * @return 成功返回true，失败返回false
 */
bool storeText(RecordPosEnu position, uint8_t *text);
```

### 数据读取函数
```c
/**
 * @brief 从Page页中组成NDEF协议的包裹
 * @param dataBuff 最终的内容缓冲区
 * @param dataBuff_MaxSize 存储缓冲区的最大长度
 * @return 成功返回HI_ERR_SUCCESS，失败返回错误码
 */
uint32_t get_NDEFDataPackage(uint8_t *dataBuff, const uint16_t dataBuff_MaxSize);
```

### 数据管理函数
```c
/**
 * @brief 清除NFC标签中的所有NDEF数据
 * @return 成功返回true，失败返回false
 */
bool nfc_clear_ndef_data(void);

/**
 * @brief 重置NFC用户数据区
 * @return 成功返回true，失败返回false
 */
bool nfc_reset_user_data(void);
```

## 使用示例

### 基本初始化
```c
#include "nfc.h"

// NFC模块初始化
uint32_t ret = nfc_init();
if (ret != HI_ERR_SUCCESS) {
    printf("NFC初始化失败: %d\n", ret);
    return;
}
printf("NFC初始化成功\n");
```

### 存储WiFi配置
```c
// 存储WiFi配置URI
uint8_t wifi_uri[] = "WIFI:S:MyWiFi;T:WPA;P:mypassword;;";
if (storeUrihttp(RECORD_POS_1, wifi_uri)) {
    printf("WiFi配置存储成功\n");
} else {
    printf("WiFi配置存储失败\n");
}

// 存储文本信息
uint8_t text_info[] = "设备信息: Hi3861 WiFi IoT设备";
if (storeText(RECORD_POS_2, text_info)) {
    printf("文本信息存储成功\n");
}
```

### 读取NFC数据
```c
uint8_t data_buffer[256];
uint32_t ret = get_NDEFDataPackage(data_buffer, sizeof(data_buffer));
if (ret == HI_ERR_SUCCESS) {
    printf("读取到NFC数据: %s\n", data_buffer);
    // 解析WiFi配置
    parse_wifi_config(data_buffer);
}
```

### 数据清除
```c
// 清除所有NDEF数据
if (nfc_clear_ndef_data()) {
    printf("NFC数据清除成功\n");
}

// 重置用户数据区
if (nfc_reset_user_data()) {
    printf("用户数据区重置成功\n");
}
```

## NDEF协议实现

### 协议结构
```c
// NDEF协议相关常量定义
#define NDEF_HEADER_SIZE 0x2           // NDEF协议的头部大小
#define NDEF_START_BYTE 0x03           // NDEF记录开始字节
#define NTAG_ERASED 0x00               // NTAG擦除状态

// NDEF协议偏移量定义
#define NDEF_PROTOCOL_HEADER_OFFSET 0           // NDEF协议头(固定)
#define NDEF_PROTOCOL_LENGTH_OFFSET 1           // NDEF协议数据的总长度位
#define NDEF_PROTOCOL_MEG_CONFIG_OFFSET 2       // 标签的控制字节位
#define NDEF_PROTOCOL_DATA_TYPE_LENGTH_OFFSET 3 // 标签数据类型的长度位
#define NDEF_PROTOCOL_DATA_LENGTH_OFFSET 4      // 标签的数据长度位
#define NDEF_PROTOCOL_DATA_TYPE_OFFSET 6        // 标签的数据类型位
#define NDEF_PROTOCOL_VALID_DATA_OFFSET 20      // 有效数据位
```

### 记录类型支持
- **URI记录**: 用于存储网址和WiFi配置
- **文本记录**: 用于存储设备信息和状态

## 内部实现

### I2C通信
模块通过I2C接口与NT3H芯片通信：
```c
// I2C读写操作封装
bool nfc_i2c_write(uint8_t reg_addr, uint8_t *data, uint16_t len);
bool nfc_i2c_read(uint8_t reg_addr, uint8_t *data, uint16_t len);
```

### 中断处理
支持NFC中断检测：
```c
// 中断引脚配置和中断处理函数
void nfc_irq_handler(void);
bool nfc_check_interrupt(void);
```

## 集成示例

### 与WiFi模块集成
```c
// NFC检测到标签时自动连接WiFi
void nfc_tag_detected_callback(uint8_t *ndef_data) {
    wifi_config_t config;
    if (parse_wifi_config_from_ndef(ndef_data, &config)) {
        wifi_connect(&config);
    }
}
```

### 与主程序集成
```c
// 在主循环中检测NFC标签
void main_task(void) {
    nfc_init();
    
    while (1) {
        // 检查NFC中断
        if (nfc_check_interrupt()) {
            uint8_t ndef_data[256];
            if (get_NDEFDataPackage(ndef_data, sizeof(ndef_data)) == HI_ERR_SUCCESS) {
                handle_nfc_data(ndef_data);
            }
        }
        osDelay(100); // 100ms检测间隔
    }
}
```

## 性能优化

### 通信优化
- **批量读写**: 减少I2C通信次数
- **缓存机制**: 本地缓存常用数据
- **中断驱动**: 避免轮询消耗CPU资源

### 内存优化
- **静态分配**: 避免动态内存分配
- **紧凑存储**: 优化NDEF数据格式
- **缓冲区复用**: 重用数据缓冲区

## 故障排除

### 常见问题
1. **初始化失败**: 检查I2C线路连接和电源
2. **读写错误**: 验证NT3H芯片型号和通信协议
3. **数据损坏**: 检查NDEF数据格式和校验

### 调试信息
```c
// 启用调试输出
#define NFC_DEBUG 1

#if NFC_DEBUG
#define nfc_printf(...) printf(__VA_ARGS__)
#else
#define nfc_printf(...)
#endif
```

## 相关文件

- `nfc.h` - NFC模块头文件，包含API声明
- `nfc.c` - NFC模块实现文件
- `NT3H.h` - NT3H芯片驱动头文件
- `NT3H.c` - NT3H芯片驱动实现
- `ndef/` - NDEF协议相关文件

## 安全考虑

- **数据验证**: NDEF数据格式验证
- **访问控制**: 防止未授权访问
- **数据加密**: 敏感信息加密存储（可选）

[返回主文档](../../../../../../../../README.md)