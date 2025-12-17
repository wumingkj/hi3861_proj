# DHT11传感器模块

## 模块概述

DHT11传感器模块提供温湿度数据的采集功能，通过单总线协议与DHT11传感器通信，实现环境温湿度的实时监测。

## 功能特性

- ✅ 单总线通信协议
- ✅ 温度和湿度同时采集
- ✅ 数据校验和错误检测
- ✅ 非阻塞式数据读取
- ✅ 自动重试机制

## 硬件配置

### 引脚定义
```c
#define DHT11_PIN         HI_IO_NAME_GPIO_7
#define DHT11_GPIO_FUN    HI_IO_FUNC_GPIO_7_GPIO
```

### 通信参数
- 通信协议：单总线
- 数据格式：40位（湿度+温度+校验和）
- 采样间隔：≥1秒

## API接口

### 传感器初始化
```c
uint8_t dht11_init(void);
```
初始化DHT11传感器，返回0表示成功。

### 数据读取
```c
uint8_t dht11_read_data(uint8_t *temp, uint8_t *humi);
```
读取温湿度数据，返回0表示读取成功。

### 底层函数
```c
void dht11_reset(void);
uint8_t dht11_check(void);
uint8_t dht11_read_byte(void);
```
用于底层通信控制的辅助函数。

## 使用示例

### 基本使用
```c
// 初始化传感器
if (dht11_init() == 0) {
    log_i("DHT11", "Sensor initialized");
}

// 读取数据
uint8_t temperature, humidity;
if (dht11_read_data(&temperature, &humidity) == 0) {
    log_i("DHT11", "Temp: %d°C, Humi: %d%%", temperature, humidity);
}
```

### 集成到主循环
```c
static void Sensor_Task(void) {
    uint32_t last_read = 0;
    
    while (1) {
        uint32_t current = Time_GetCurrentMs();
        
        // 每2秒读取一次
        if (current - last_read >= 2000) {
            uint8_t temp, humi;
            if (dht11_read_data(&temp, &humi) == 0) {
                // 处理数据...
            }
            last_read = current;
        }
        
        Time_DelayMs(100);
    }
}
```

## 内部实现

### 通信协议
DHT11使用单总线协议：
1. **主机启动**：拉低总线至少18ms
2. **传感器响应**：拉低80μs后拉高80μs
3. **数据传输**：40位数据，每位以50μs低电平开始

### 数据格式
湿度整数(8bit) + 湿度小数(8bit) + 温度整数(8bit) + 温度小数(8bit) + 校验和(8bit)

[返回主文档](../../../../../../../../README.md)