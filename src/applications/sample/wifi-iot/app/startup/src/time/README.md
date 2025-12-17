# 时间管理模块

## 模块概述

时间管理模块提供系统时间管理和精确延时控制功能，封装了CMSIS-RTOS2的时间相关API，提供统一易用的时间操作接口。

## 功能特性

- ✅ 系统时间获取和管理
- ✅ 精确的毫秒级延时
- ✅ 非阻塞时间间隔检查
- ✅ 多精度延时支持
- ✅ 系统tick频率适配

## API接口

### 初始化函数
```c
void Time_Init(void);
```
初始化时间库，获取系统tick频率。

### 时间获取
```c
uint32_t Time_GetCurrentMs(void);
```
获取当前系统时间（毫秒）。

### 延时函数
```c
void Time_DelayMs(uint32_t ms);
void Time_DelayMsPrecise(uint32_t ms);
```
任务延时和精确延时函数。

### 间隔检查
```c
bool Time_CheckInterval(uint32_t *last_time, uint32_t interval_ms);
```
检查是否达到指定时间间隔。

### 系统信息
```c
uint32_t Time_GetTickFrequency(void);
```
获取系统tick频率。

## 使用示例

### 基本时间操作
```c
// 初始化时间库
Time_Init();

// 获取当前时间
uint32_t start_time = Time_GetCurrentMs();

// 延时500毫秒
Time_DelayMs(500);

// 计算经过时间
uint32_t elapsed = Time_GetCurrentMs() - start_time;
```

### 周期性任务
```c
static void Periodic_Task(void) {
    uint32_t last_execution = 0;
    
    while (1) {
        // 每1秒执行一次
        if (Time_CheckInterval(&last_execution, 1000)) {
            // 执行周期性任务
            log_i("TASK", "Periodic task executed");
        }
        
        // 其他任务...
        Time_DelayMs(10);
    }
}
```

### 精确控制
```c
// 需要高精度延时时使用
Time_DelayMsPrecise(5);  // 精确延时5毫秒
```

## 内部实现

### 时间基准
模块使用系统tick作为时间基准：
```c
g_tick_freq = osKernelGetTickCount();
current_ms = (osKernelGetTickCount() * 1000) / g_tick_freq;
```

### 延时实现
- **任务延时**：使用`osDelay`，适合长延时
- **精确延时**：使用`usleep`，适合短延时

### 频率适配
自动检测系统tick频率，支持不同配置的系统。

## 性能特点

### 精度对比
| 延时方式 | 精度 | 适用场景 |
|---------|------|----------|
| `Time_DelayMs` | 中等 | 一般任务延时 |
| `Time_DelayMsPrecise` | 高 | 精确时序控制 |

### 资源使用
- 零动态内存分配
- 静态变量存储配置
- 内联函数优化

## 错误处理

### 边界条件
- 零延时处理
- 未初始化检测
- 溢出保护

### 安全机制
```c
if (g_tick_freq == 0) {
    return 0;  // 未初始化保护
}
```

## 集成建议

### 主循环设计
```c
void Main_Loop(void) {
    Time_Init();
    uint32_t last_sensor = 0, last_display = 0;
    
    while (1) {
        // 传感器读取（2秒间隔）
        if (Time_CheckInterval(&last_sensor, 2000)) {
            // 读取传感器...
        }
        
        // 显示更新（100毫秒间隔）
        if (Time_CheckInterval(&last_display, 100)) {
            // 更新显示...
        }
        
        Time_DelayMs(5);
    }
}
```

### 多任务协调
- 使用相同的时间基准
- 避免长时间阻塞
- 合理设置任务优先级

## 调试支持

### 时间统计
```c
uint32_t start = Time_GetCurrentMs();
// 执行操作...
uint32_t duration = Time_GetCurrentMs() - start;
log_d("PERF", "Operation took %lu ms", duration);
```

### 频率监控
```c
log_i("TIME", "System tick frequency: %lu Hz", Time_GetTickFrequency());
```

## 相关文件

- `time.h` - 头文件，包含API声明
- `time.c` - 实现文件，包含时间管理逻辑

[返回主文档](../README.md)