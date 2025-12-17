# 蜂鸣器模块

## 模块概述

蜂鸣器模块提供基于PWM的蜂鸣器控制功能，支持多种报警模式和精确的时间控制。该模块采用非阻塞设计，适合在实时系统中使用。

## 功能特性

- ✅ PWM硬件控制蜂鸣器
- ✅ 单次蜂鸣和重复报警模式
- ✅ 非阻塞操作，立即返回
- ✅ 精确的时间控制
- ✅ 状态查询和强制停止

## 硬件配置

### 引脚定义
```c
#define BEEP_PIN              HI_IO_NAME_GPIO_14
#define BEEP_PIN_PWM_FUNC     HI_IO_FUNC_GPIO_14_PWM5_OUT
#define BEEP_PWM_PORT         HI_PWM_PORT_PWM5
```

### PWM参数
```c
#define BEEP_PWM_PERIOD       40000  // PWM周期
#define BEEP_PWM_DUTY         20000  // PWM占空比
```

## API接口

### 初始化函数
```c
void Buzzer_Init(void);
```
初始化蜂鸣器硬件和PWM控制器。

### 蜂鸣控制
```c
void Buzzer_BeepMs(uint16_t ms);
```
蜂鸣指定毫秒数（非阻塞）。

### 报警模式
```c
void Buzzer_Alarm(uint16_t cnt, uint16_t on_ms, uint16_t off_ms);
```
重复报警模式，可设置次数、响铃时间和间隔时间。

### 状态控制
```c
void Buzzer_Stop(void);
bool Buzzer_IsBusy(void);
```
停止蜂鸣和查询蜂鸣器状态。

### 定时器回调
```c
void Buzzer_Tick(uint32_t now_ms);
```
需要在主循环中定期调用（建议1-5ms间隔）。

## 使用示例

### 基本使用
```c
// 初始化
Buzzer_Init();

// 单次蜂鸣500ms
Buzzer_BeepMs(500);

// 报警模式：响3次，每次1秒，间隔500ms
Buzzer_Alarm(3, 1000, 500);
```

### 集成到主循环
```c
// 在主任务循环中调用
while (1) {
    uint32_t current_time = Time_GetCurrentMs();
    Buzzer_Tick(current_time);
    // ... 其他任务
    Time_DelayMs(5);
}
```

## 内部实现

### 状态机设计
模块使用状态机管理蜂鸣器状态：
- `BUZ_IDLE` - 空闲状态
- `BUZ_ON` - 蜂鸣状态
- `BUZ_OFF_GAP` - 间隔状态

### 非阻塞机制
通过`Buzzer_Tick`函数实现非阻塞时间控制，避免长时间延时阻塞系统。

## 错误处理

- 参数验证：检查输入参数的有效性
- 状态保护：防止非法状态转换
- 日志输出：详细的调试信息

## 性能优化

- 最小化PWM操作频率
- 使用静态变量减少内存分配
- 优化的状态切换逻辑

## 注意事项

1. **定时器调用**：必须定期调用`Buzzer_Tick`函数
2. **PWM资源**：确保PWM端口未被其他模块占用
3. **中断安全**：模块非中断安全，需在任务上下文中使用

## 相关文件

- `buzzer.h` - 头文件，包含API声明和配置
- `buzzer.c` - 实现文件，包含完整功能实现

[返回主文档](../../../../../../../../README.md)