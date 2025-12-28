# 时间管理模块

## 模块概述

时间管理模块提供系统时间管理和精确延时控制功能，封装了CMSIS-RTOS2的时间相关API，提供统一易用的时间操作接口。该模块支持毫秒级时间获取、精确延时、周期性任务调度等功能，是系统时序控制的核心组件。

## 功能特性

- ✅ **系统时间管理** - 统一的毫秒级时间基准
- ✅ **精确延时控制** - 毫秒级和微秒级延时支持
- ✅ **非阻塞时间检查** - 高效的时间间隔检测
- ✅ **多精度延时支持** - 适应不同精度需求
- ✅ **系统tick频率适配** - 自动适配不同系统配置
- ✅ **周期性任务调度** - 简化定时任务实现
- ✅ **性能监控** - 执行时间统计和性能分析

## API接口

### 初始化函数
```c
/**
 * @brief 初始化时间管理模块
 * @note 必须在其他时间函数调用前执行
 */
void Time_Init(void);
```

### 时间获取函数
```c
/**
 * @brief 获取当前系统时间（毫秒）
 * @return 从系统启动开始的毫秒数
 */
uint32_t Time_GetCurrentMs(void);

/**
 * @brief 获取系统tick频率
 * @return 系统tick频率（Hz）
 */
uint32_t Time_GetTickFrequency(void);
```

### 延时控制函数
```c
/**
 * @brief 任务延时（毫秒）
 * @param ms 延时时间（毫秒）
 * @note 使用osDelay实现，适合长延时
 */
void Time_DelayMs(uint32_t ms);

/**
 * @brief 精确延时（毫秒）
 * @param ms 延时时间（毫秒）
 * @note 使用usleep实现，适合短延时和精确时序控制
 */
void Time_DelayMsPrecise(uint32_t ms);

/**
 * @brief 微秒级精确延时
 * @param us 延时时间（微秒）
 */
void Time_DelayUs(uint32_t us);
```

### 时间间隔检查函数
```c
/**
 * @brief 检查是否达到指定时间间隔
 * @param last_time 上次执行时间指针
 * @param interval_ms 时间间隔（毫秒）
 * @return true表示达到间隔，false表示未达到
 * @note 会自动更新last_time，适合周期性任务
 */
bool Time_CheckInterval(uint32_t *last_time, uint32_t interval_ms);

/**
 * @brief 检查时间间隔（不自动更新）
 * @param last_time 上次执行时间
 * @param interval_ms 时间间隔（毫秒）
 * @return true表示达到间隔
 */
bool Time_IsIntervalElapsed(uint32_t last_time, uint32_t interval_ms);
```

### 时间计算函数
```c
/**
 * @brief 计算时间差（毫秒）
 * @param start_time 开始时间
 * @param end_time 结束时间
 * @return 时间差（毫秒）
 */
uint32_t Time_CalculateElapsed(uint32_t start_time, uint32_t end_time);

/**
 * @brief 检查超时
 * @param start_time 开始时间
 * @param timeout_ms 超时时间（毫秒）
 * @return true表示已超时
 */
bool Time_IsTimeout(uint32_t start_time, uint32_t timeout_ms);
```

## 使用示例

### 基本时间操作
```c
#include "time.h"

// 初始化时间库（必须在其他时间函数前调用）
Time_Init();

// 获取系统信息
uint32_t tick_freq = Time_GetTickFrequency();
printf("系统tick频率: %lu Hz\n", tick_freq);

// 获取当前时间
uint32_t start_time = Time_GetCurrentMs();
printf("系统启动时间: %lu ms\n", start_time);

// 延时500毫秒
Time_DelayMs(500);

// 计算经过时间
uint32_t elapsed_time = Time_GetCurrentMs() - start_time;
printf("经过时间: %lu ms\n", elapsed_time);
```

### 周期性任务实现
```c
// 周期性传感器读取任务
static void Sensor_Read_Task(void) {
    uint32_t last_sensor_read = 0;    // 传感器上次读取时间
    uint32_t last_display_update = 0; // 显示上次更新时间
    
    while (1) {
        // 每2秒读取一次传感器（非阻塞方式）
        if (Time_CheckInterval(&last_sensor_read, 2000)) {
            uint8_t temp, humi;
            if (dht11_read_data(&temp, &humi) == 0) {
                printf("传感器数据: 温度=%d°C, 湿度=%d%%\n", temp, humi);
            }
        }
        
        // 每100毫秒更新一次显示
        if (Time_CheckInterval(&last_display_update, 100)) {
            update_display();
        }
        
        // 短暂延时，避免过度占用CPU
        Time_DelayMs(10);
    }
}
```

### 精确时序控制
```c
// 需要高精度时序控制的应用
void precise_timing_example(void) {
    // 初始化时间模块
    Time_Init();
    
    // 精确控制LED闪烁
    while (1) {
        // LED亮500毫秒（精确控制）
        led_set_state(LED_ON);
        Time_DelayMsPrecise(500);
        
        // LED灭500毫秒（精确控制）
        led_set_state(LED_OFF);
        Time_DelayMsPrecise(500);
    }
}

// 通信协议时序控制
void communication_timing(void) {
    // 发送起始位（精确时序）
    gpio_set_high();
    Time_DelayUs(50);  // 50微秒高电平
    
    // 发送数据位
    for (int i = 0; i < 8; i++) {
        gpio_set_low();
        Time_DelayUs(10);  // 10微秒低电平
        
        // 设置数据位
        if (data & (1 << i)) {
            gpio_set_high();
        } else {
            gpio_set_low();
        }
        Time_DelayUs(40);  // 40微秒数据位
    }
}
```

### 超时检测
```c
// 带超时检测的等待函数
bool wait_for_event_with_timeout(uint32_t timeout_ms) {
    uint32_t start_time = Time_GetCurrentMs();
    
    while (!check_event()) {
        // 检查是否超时
        if (Time_IsTimeout(start_time, timeout_ms)) {
            printf("等待事件超时\n");
            return false;
        }
        
        // 短暂延时后继续检查
        Time_DelayMs(10);
    }
    
    printf("事件已发生，等待时间: %lu ms\n", 
           Time_CalculateElapsed(start_time, Time_GetCurrentMs()));
    return true;
}
```

### 性能监控
```c
// 函数执行时间统计
void performance_monitoring_example(void) {
    uint32_t start_time, end_time, duration;
    
    // 测试传感器读取性能
    start_time = Time_GetCurrentMs();
    for (int i = 0; i < 10; i++) {
        dht11_read_data(&temp, &humi);
        Time_DelayMs(100); // 避免传感器限制
    }
    end_time = Time_GetCurrentMs();
    
    duration = Time_CalculateElapsed(start_time, end_time);
    printf("10次传感器读取耗时: %lu ms，平均每次: %lu ms\n", 
           duration, duration / 10);
}
```

## 内部实现

### 时间基准系统
模块使用系统tick作为时间基准，自动适配不同频率配置：
```c
// 全局时间基准
static uint32_t g_tick_freq = 0;

void Time_Init(void) {
    g_tick_freq = osKernelGetTickFreq();
    if (g_tick_freq == 0) {
        // 默认频率（通常为1000Hz）
        g_tick_freq = 1000;
    }
}

uint32_t Time_GetCurrentMs(void) {
    if (g_tick_freq == 0) {
        return 0;  // 未初始化保护
    }
    return (osKernelGetTickCount() * 1000) / g_tick_freq;
}
```

### 延时实现机制

#### 任务延时（Time_DelayMs）
```c
void Time_DelayMs(uint32_t ms) {
    if (ms == 0) return;
    
    // 转换为tick数
    uint32_t ticks = (ms * g_tick_freq) / 1000;
    if (ticks == 0) ticks = 1;  // 至少延时1个tick
    
    osDelay(ticks);
}
```

#### 精确延时（Time_DelayMsPrecise）
```c
void Time_DelayMsPrecise(uint32_t ms) {
    if (ms == 0) return;
    
    // 使用usleep实现微秒级精度
    usleep(ms * 1000);
}
```

### 时间间隔检查算法
```c
bool Time_CheckInterval(uint32_t *last_time, uint32_t interval_ms) {
    uint32_t current = Time_GetCurrentMs();
    uint32_t elapsed = current - *last_time;
    
    if (elapsed >= interval_ms) {
        *last_time = current;  // 更新上次时间
        return true;
    }
    return false;
}
```

## 性能特点

### 精度对比表
| 延时方式 | 精度 | 适用场景 | 资源消耗 |
|---------|------|----------|----------|
| `Time_DelayMs` | 中等（±1 tick） | 一般任务延时 | 低 |
| `Time_DelayMsPrecise` | 高（微秒级） | 精确时序控制 | 中 |
| `Time_DelayUs` | 最高（微秒级） | 硬件时序控制 | 高 |

### 资源优化策略
- **零动态内存分配** - 所有变量静态存储
- **内联函数优化** - 关键函数内联实现
- **缓存友好设计** - 局部性原理优化
- **最小化系统调用** - 减少上下文切换

## 错误处理与安全机制

### 边界条件处理
```c
// 零延时处理
if (ms == 0) return;

// 未初始化检测
if (g_tick_freq == 0) {
    // 尝试自动初始化
    Time_Init();
    if (g_tick_freq == 0) {
        return;  // 初始化失败
    }
}

// 溢出保护
if (current_time < *last_time) {
    // 处理时间回绕（32位溢出）
    *last_time = current_time;
    return true;
}
```

### 安全机制
- **参数验证** - 所有输入参数范围检查
- **状态保护** - 防止未初始化使用
- **异常恢复** - 自动恢复错误状态

## 集成建议

### 主循环设计模式
```c
void Main_Loop(void) {
    // 初始化时间模块
    Time_Init();
    
    // 各任务的上次执行时间
    uint32_t last_sensor = 0, last_display = 0;
    uint32_t last_network = 0, last_nfc = 0;
    
    while (1) {
        // 传感器任务（2秒间隔）
        if (Time_CheckInterval(&last_sensor, 2000)) {
            sensor_task();
        }
        
        // 显示更新任务（100毫秒间隔）
        if (Time_CheckInterval(&last_display, 100)) {
            display_task();
        }
        
        // 网络状态检查（5秒间隔）
        if (Time_CheckInterval(&last_network, 5000)) {
            network_task();
        }
        
        // NFC检测任务（500毫秒间隔）
        if (Time_CheckInterval(&last_nfc, 500)) {
            nfc_task();
        }
        
        // 主循环延时（避免过度占用CPU）
        Time_DelayMs(5);
    }
}
```

### 多任务协调策略
- **统一时间基准** - 所有任务使用相同的时间源
- **优先级分配** - 根据任务重要性设置检查频率
- **负载均衡** - 避免多个任务同时执行
- **错误隔离** - 单个任务失败不影响其他任务

## 调试与性能分析

### 时间统计功能
```c
// 性能监控宏
#define TIME_MEASURE_START() uint32_t _start_time = Time_GetCurrentMs()
#define TIME_MEASURE_END(tag) \
    do { \
        uint32_t _end_time = Time_GetCurrentMs(); \
        uint32_t _duration = _end_time - _start_time; \
        printf("[PERF] %s: %lu ms\n", tag, _duration); \
    } while(0)

// 使用示例
void monitored_function(void) {
    TIME_MEASURE_START();
    
    // 执行需要监控的代码
    complex_operation();
    
    TIME_MEASURE_END("complex_operation");
}
```

### 系统状态监控
```c
// 系统时间信息输出
void print_time_info(void) {
    printf("时间模块信息:\n");
    printf("  Tick频率: %lu Hz\n", Time_GetTickFrequency());
    printf("  当前时间: %lu ms\n", Time_GetCurrentMs());
    printf("  系统运行: %lu 秒\n", Time_GetCurrentMs() / 1000);
}
```

## 相关文件

- `time.h` - 时间管理模块头文件，包含所有API声明
- `time.c` - 时间管理模块实现文件，包含完整逻辑
- `README.md` - 模块说明文档

## 依赖关系

- **CMSIS-RTOS2** - 操作系统时间接口
- **标准C库** - 基本时间函数支持

## 扩展功能

### 未来扩展计划
- 🔄 **高精度计时器** - 纳秒级精度支持
- 🔄 **时间同步** - NTP网络时间同步
- 🔄 **日历功能** - 日期时间管理
- 🔄 **定时器管理** - 多个软件定时器支持

[返回主文档](../../../../../../../../README.md)