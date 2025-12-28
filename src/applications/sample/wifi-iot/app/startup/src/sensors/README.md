# 传感器模块

## 模块概述

传感器模块提供环境数据的采集功能，目前支持DHT11温湿度传感器和MQ-2烟雾传感器。该模块通过不同的通信协议与传感器通信，实现环境温湿度和烟雾浓度的实时监测，并提供数据校验、错误检测和自动重试机制。

## 功能特性

### DHT11温湿度传感器
- ✅ **单总线通信协议** - 标准DHT11通信协议实现
- ✅ **温度和湿度同时采集** - 一次读取获取完整环境数据
- ✅ **数据校验和错误检测** - 内置CRC校验确保数据准确性
- ✅ **非阻塞式数据读取** - 支持异步数据采集
- ✅ **自动重试机制** - 通信失败时自动重试

### MQ-2烟雾传感器
- ✅ **ADC模拟信号采集** - 高精度ADC转换
- ✅ **多级浓度检测** - 支持5个浓度等级检测
- ✅ **可配置阈值** - 灵活的报警阈值设置
- ✅ **电压值转换** - 原始ADC值转换为实际电压
- ✅ **报警功能** - 烟雾浓度超标自动报警
- ✅ **实时监测** - 连续烟雾浓度监测

## 硬件配置

### DHT11温湿度传感器
- **型号**: DHT11温湿度传感器
- **温度范围**: 0-50°C (±2°C精度)
- **湿度范围**: 20-90%RH (±5%RH精度)
- **供电电压**: 3.3V-5.5V
- **接口类型**: 单总线数字信号

#### 引脚定义
```c
#define DHT11_PIN         HI_IO_NAME_GPIO_7    // DHT11数据引脚
#define DHT11_GPIO_FUN    HI_IO_FUNC_GPIO_7_GPIO // GPIO功能配置
```

### MQ-2烟雾传感器
- **型号**: MQ-2烟雾/可燃气体传感器
- **检测气体**: 烟雾、液化气、丙烷、氢气等
- **工作电压**: 5V DC
- **输出信号**: 模拟电压信号 (0-5V)
- **预热时间**: 约20秒

#### 引脚定义
```c
#define SMOKE_SENSOR_PIN         HI_IO_NAME_GPIO_11  // 使用ADC5通道
#define ADC5_PIN                 HI_IO_NAME_GPIO_11 // ADC5引脚定义
```

#### 浓度阈值定义
```c
#define SMOKE_THRESHOLD_LOW      500     // 低浓度阈值
#define SMOKE_THRESHOLD_MEDIUM   1000    // 中浓度阈值
#define SMOKE_THRESHOLD_HIGH     1500    // 高浓度阈值
#define SMOKE_THRESHOLD_DANGER   2000    // 危险浓度阈值
```

## API接口

### DHT11传感器接口

#### 初始化函数
```c
uint8_t dht11_init(void);
```

#### 数据读取函数
```c
uint8_t dht11_read_data(uint8_t *temp, uint8_t *humi);
uint8_t dht11_read_data_with_retry(uint8_t *temp, uint8_t *humi, uint8_t max_retries);
```

#### 底层通信函数
```c
void dht11_reset(void);
uint8_t dht11_check(void);
uint8_t dht11_read_byte(void);
```

### MQ-2烟雾传感器接口

#### 初始化函数
```c
void smoke_sensor_init(void);
void adc5_init(void);
```

#### 数据读取函数
```c
uint16_t smoke_sensor_read_raw(void);
float smoke_sensor_read_voltage(void);
smoke_level_t smoke_sensor_get_level(void);
void smoke_sensor_read_data(smoke_sensor_data_t *data);
```

#### 报警和配置函数
```c
bool smoke_sensor_check_alarm(uint16_t threshold);
void smoke_sensor_set_threshold(smoke_level_t level, uint16_t threshold);
const char* smoke_sensor_get_level_string(smoke_level_t level);
```

#### 测试函数
```c
void smoke_sensor_test(void);
```

### 数据类型定义

#### 烟雾传感器数据结构
```c
typedef struct {
    uint16_t raw_value;          // 原始ADC值
    float voltage;               // 电压值
    smoke_level_t level;         // 烟雾浓度等级
    uint32_t timestamp;          // 时间戳
    bool alarm_triggered;        // 报警是否触发
} smoke_sensor_data_t;
```

#### 烟雾浓度等级枚举
```c
typedef enum {
    SMOKE_LEVEL_NONE = 0,        // 无烟雾
    SMOKE_LEVEL_LOW,             // 低浓度
    SMOKE_LEVEL_MEDIUM,          // 中浓度
    SMOKE_LEVEL_HIGH,            // 高浓度
    SMOKE_LEVEL_DANGER           // 危险浓度
} smoke_level_t;
```

## 使用示例

### DHT11温湿度传感器使用

#### 基本使用
```c
#include "dht11.h"

// 初始化传感器
if (dht11_init() == 0) {
    printf("DHT11传感器初始化成功\n");
}

// 读取温湿度数据
uint8_t temperature, humidity;
if (dht11_read_data(&temperature, &humidity) == 0) {
    printf("温度: %d°C, 湿度: %d%%\n", temperature, humidity);
}
```

#### 带重试的数据读取
```c
uint8_t temp, humi;
if (dht11_read_data_with_retry(&temp, &humi, 3) == 0) {
    printf("读取成功: 温度=%d°C, 湿度=%d%%\n", temp, humi);
}
```

### MQ-2烟雾传感器使用

#### 基本使用
```c
#include "smoke.h"

// 初始化烟雾传感器
smoke_sensor_init();

// 读取原始ADC值
uint16_t raw_value = smoke_sensor_read_raw();
printf("烟雾传感器原始值: %d\n", raw_value);

// 读取电压值
float voltage = smoke_sensor_read_voltage();
printf("烟雾传感器电压: %.2f V\n", voltage);

// 获取浓度等级
smoke_level_t level = smoke_sensor_get_level();
printf("烟雾浓度等级: %s\n", smoke_sensor_get_level_string(level));
```

#### 完整数据读取
```c
smoke_sensor_data_t smoke_data;
smoke_sensor_read_data(&smoke_data);

printf("烟雾传感器数据:\n");
printf("  原始值: %d\n", smoke_data.raw_value);
printf("  电压: %.2f V\n", smoke_data.voltage);
printf("  浓度等级: %s\n", smoke_sensor_get_level_string(smoke_data.level));
printf("  时间戳: %lu\n", smoke_data.timestamp);
printf("  报警状态: %s\n", smoke_data.alarm_triggered ? "已触发" : "未触发");
```

#### 报警检测
```c
// 检查是否触发报警
if (smoke_sensor_check_alarm(SMOKE_THRESHOLD_HIGH)) {
    printf("烟雾浓度超标！触发报警\n");
    // 触发蜂鸣器报警
    buzzer_alarm_mode(1000, 500, 1000, 3);
} else {
    printf("烟雾浓度正常\n");
}
```

### 多传感器集成示例

#### 环境监测任务
```c
static void Environment_Monitor_Task(void) {
    uint32_t last_dht11_read = 0;
    uint32_t last_smoke_read = 0;
    
    // 初始化所有传感器
    dht11_init();
    smoke_sensor_init();
    
    while (1) {
        // 每2秒读取一次温湿度
        if (Time_CheckInterval(&last_dht11_read, 2000)) {
            uint8_t temp, humi;
            if (dht11_read_data(&temp, &humi) == 0) {
                printf("环境数据 - 温度: %d°C, 湿度: %d%%\n", temp, humi);
            }
        }
        
        // 每1秒读取一次烟雾浓度
        if (Time_CheckInterval(&last_smoke_read, 1000)) {
            smoke_sensor_data_t smoke_data;
            smoke_sensor_read_data(&smoke_data);
            
            printf("安全数据 - 烟雾等级: %s, ADC值: %d\n", 
                   smoke_sensor_get_level_string(smoke_data.level),
                   smoke_data.raw_value);
            
            // 检查烟雾报警
            if (smoke_sensor_check_alarm(SMOKE_THRESHOLD_HIGH)) {
                printf("警告！检测到高浓度烟雾！\n");
                // 触发声光报警
                buzzer_alarm_mode(500, 200, 500, 5);
                led_set_state(LED_BLINK);
            }
        }
        
        Time_DelayMs(100);
    }
}
```

#### 与PHP API集成
```c
// 通过HTTP API发布传感器数据
void publish_sensor_data_via_api(void) {
    uint8_t temperature, humidity;
    smoke_sensor_data_t smoke_data;
    
    // 读取所有传感器数据
    dht11_read_data(&temperature, &humidity);
    smoke_sensor_read_data(&smoke_data);
    
    // 构建JSON数据
    char json_data[256];
    snprintf(json_data, sizeof(json_data),
        "{\"temperature\":%d,\"humidity\":%d,\"smoke_level\":%d,\"smoke_value\":%d}",
        temperature, humidity, smoke_data.level, smoke_data.raw_value);
    
    // 通过PHP API发布数据
    php_api_publish_sensor_data(json_data);
}
```

## 内部实现

### DHT11通信协议

DHT11使用单总线协议，通信流程如下：
1. **主机启动信号**：拉低总线至少18ms
2. **传感器响应**：拉低80μs后拉高80μs
3. **数据传输**：40位数据，每位以50μs低电平开始

#### 数据格式
```c
// DHT11数据格式（40位 = 5字节）
typedef struct {
    uint8_t humidity_int;     // 湿度整数部分
    uint8_t humidity_frac;    // 湿度小数部分
    uint8_t temperature_int;  // 温度整数部分
    uint8_t temperature_frac; // 温度小数部分
    uint8_t checksum;         // 校验和
} dht11_data_t;
```

### MQ-2烟雾传感器ADC采集

#### ADC初始化
```c
void adc5_init(void) {
    // 配置GPIO为ADC功能
    hi_io_set_func(ADC5_PIN, HI_IO_FUNC_GPIO_11_ADC5);
    
    // 初始化ADC通道5
    hi_adc_init(HI_ADC_CHANNEL_5, HI_ADC_CUR_BAIS_DEFAULT, HI_ADC_ATTR_SINGLE);
}
```

#### 数据转换
```c
float get_adc5_voltage(void) {
    uint16_t adc_value = get_adc5_value();
    // ADC值转换为电压 (0-3.3V)
    return (adc_value * 3.3f) / 4095.0f;
}
```

#### 浓度等级判断
```c
smoke_level_t smoke_sensor_get_level(void) {
    uint16_t raw_value = smoke_sensor_read_raw();
    
    if (raw_value < SMOKE_THRESHOLD_LOW) {
        return SMOKE_LEVEL_NONE;
    } else if (raw_value < SMOKE_THRESHOLD_MEDIUM) {
        return SMOKE_LEVEL_LOW;
    } else if (raw_value < SMOKE_THRESHOLD_HIGH) {
        return SMOKE_LEVEL_MEDIUM;
    } else if (raw_value < SMOKE_THRESHOLD_DANGER) {
        return SMOKE_LEVEL_HIGH;
    } else {
        return SMOKE_LEVEL_DANGER;
    }
}
```

## 性能优化

### 时序精度优化
- **微秒级延时**：使用高精度延时函数确保时序准确
- **信号边沿检测**：精确检测信号边沿变化
- **超时保护**：防止通信死锁

### 数据可靠性
- **多重校验**：信号时序校验 + 数据校验和
- **数据过滤**：排除明显异常数据
- **历史数据**：必要时使用历史数据替代

### 资源优化
- **最小化延时**：优化通信时序减少等待时间
- **状态缓存**：缓存传感器状态减少重复初始化
- **错误恢复**：自动恢复通信错误

## 故障排除

### DHT11常见问题
1. **初始化失败**
   - 检查电源连接（3.3V-5V）
   - 验证GPIO引脚配置
   - 检查上拉电阻（建议4.7KΩ）

2. **读取数据失败**
   - 检查通信线路是否稳定
   - 增加重试次数
   - 调整时序参数

### MQ-2常见问题
1. **ADC值异常**
   - 检查传感器供电（5V）
   - 验证ADC引脚配置
   - 检查传感器预热时间（约20秒）

2. **浓度判断不准确**
   - 根据实际环境调整阈值
   - 检查传感器校准
   - 考虑环境温度补偿

### 调试支持
```c
// 启用调试输出
#define SENSORS_DEBUG 1

#if SENSORS_DEBUG
#define sensors_printf(...) printf("[SENSORS] " __VA_ARGS__)
#else
#define sensors_printf(...)
#endif
```

## 相关文件

- `dht11.h` - DHT11温湿度传感器头文件
- `dht11.c` - DHT11温湿度传感器实现文件
- `smoke.h` - MQ-2烟雾传感器头文件
- `smoke.c` - MQ-2烟雾传感器实现文件
- `adc.h` - ADC驱动头文件
- `adc.c` - ADC驱动实现文件
- `README.md` - 模块说明文档

## 扩展性

### 支持更多传感器
模块设计支持扩展其他传感器类型：
```c
// 传感器类型枚举
typedef enum {
    SENSOR_TYPE_DHT11,
    SENSOR_TYPE_MQ2,
    SENSOR_TYPE_DHT22,
    SENSOR_TYPE_DS18B20,
    SENSOR_TYPE_BMP280
} sensor_type_t;
```

### 未来扩展计划
- 🔄 **更多气体传感器**：MQ-5、MQ-7等
- 🔄 **光照传感器**：BH1750等
- 🔄 **运动传感器**：MPU6050等
- 🔄 **统一传感器接口**：标准化数据格式

[返回主文档](../../../../../../../../README.md)