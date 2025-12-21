# 高级按键管理器

## 模块概述

高级按键管理器提供基于状态机的按键检测功能，支持多种按键事件检测，包括短按、长按、双击等。该模块采用动态内存分配，支持多按键管理，具有高度的灵活性和可配置性。

## 功能特性

- ✅ **多按键支持**：动态管理多个按键，支持添加/移除按键
- ✅ **多种事件检测**：支持按下、释放、短按、长按、双击、保持按下等事件
- ✅ **可配置参数**：支持自定义消抖时间、长按时间、双击间隔等参数
- ✅ **状态查询**：提供按键状态、按下持续时间等查询功能
- ✅ **非阻塞设计**：通过定期更新实现非阻塞按键检测
- ✅ **消抖处理**：内置硬件消抖机制，防止误触发

## 硬件配置

### 默认引脚定义
```c
#define KEY1_PIN HI_IO_NAME_GPIO_11  // 按键1引脚
#define KEY2_PIN HI_IO_NAME_GPIO_12  // 按键2引脚
```

### 默认参数配置
```c
// 默认消抖时间：20ms
// 默认长按时间：1000ms  
// 默认双击间隔：300ms
```

## API接口

### 管理器创建和销毁
```c
key_manager_t* key_manager_create(uint8_t max_keys);
void key_manager_destroy(key_manager_t *manager);
```
创建和销毁按键管理器实例。

### 按键管理
```c
int8_t key_manager_add_key(key_manager_t *manager, hi_io_name pin, uint8_t func,
                          uint32_t debounce_time, uint32_t long_press_time, uint32_t double_click_time);
int8_t key_manager_remove_key(key_manager_t *manager, uint8_t key_index);
```
添加和移除按键配置。

### 状态更新和事件检测
```c
void key_manager_update(key_manager_t *manager);
key_event_t key_manager_get_event(key_manager_t *manager, uint8_t key_index);
```
更新按键状态和获取按键事件。

### 状态查询
```c
bool key_manager_is_pressed(key_manager_t *manager, uint8_t key_index);
bool key_manager_is_holding(key_manager_t *manager, uint8_t key_index);
uint32_t key_manager_get_press_duration(key_manager_t *manager, uint8_t key_index);
```
查询按键状态和按下持续时间。

### 配置管理
```c
const key_config_t* key_manager_get_config(key_manager_t *manager, uint8_t key_index);
int8_t key_manager_set_config(key_manager_t *manager, uint8_t key_index,
                             uint32_t debounce_time, uint32_t long_press_time, uint32_t double_click_time);
void key_manager_reset(key_manager_t *manager, uint8_t key_index);
```
获取和修改按键配置，重置按键状态。

## 使用示例

### 基本使用
```c
// 创建按键管理器（支持2个按键）
key_manager_t *key_mgr = key_manager_create(2);

// 添加按键1（使用默认参数）
key_manager_add_key(key_mgr, HI_IO_NAME_GPIO_11, 0, 0, 0, 0);

// 添加按键2（自定义参数）
key_manager_add_key(key_mgr, HI_IO_NAME_GPIO_12, 0, 30, 1500, 500);

// 在主循环中定期更新
while (1) {
    key_manager_update(key_mgr);
    
    // 检测按键1事件
    key_event_t event1 = key_manager_get_event(key_mgr, 0);
    if (event1 == KEY_EVENT_SHORT_PRESS) {
        printf("按键1短按\n");
    } else if (event1 == KEY_EVENT_LONG_PRESS) {
        printf("按键1长按\n");
    }
    
    Time_DelayMs(5);
}

// 销毁管理器
key_manager_destroy(key_mgr);
```

### 集成到定时器回调
```c
// 定时器回调函数（5ms周期）
void KeyTimerCb(void *arg) {
    key_manager_t *key_mgr = (key_manager_t*)arg;
    key_manager_update(key_mgr);
    
    // 处理所有按键事件
    for (uint8_t i = 0; i < key_mgr->key_count; i++) {
        key_event_t event = key_manager_get_event(key_mgr, i);
        handle_key_event(i, event);
    }
}
```

## 事件类型说明

| 事件类型 | 说明 | 触发条件 |
|---------|------|----------|
| `KEY_EVENT_NONE` | 无事件 | 默认状态 |
| `KEY_EVENT_PRESS` | 按下事件 | 按键按下时触发 |
| `KEY_EVENT_RELEASE` | 释放事件 | 按键释放时触发 |
| `KEY_EVENT_SHORT_PRESS` | 短按事件 | 按下后快速释放 |
| `KEY_EVENT_LONG_PRESS` | 长按事件 | 按下时间超过长按阈值 |
| `KEY_EVENT_DOUBLE_CLICK` | 双击事件 | 快速连续按下两次 |
| `KEY_EVENT_HOLD` | 保持按下 | 按键持续按下状态 |

## 内部实现

### 状态机设计
模块使用三状态状态机：
- `KEY_STATE_RELEASED` - 释放状态
- `KEY_STATE_DEBOUNCING` - 消抖状态
- `KEY_STATE_PRESSED` - 按下状态

### 消抖机制
采用硬件消抖，通过检测按键状态变化并等待消抖时间确认有效按键。

### 双击检测
通过记录两次按键释放的时间间隔来判断是否为双击操作。

## 性能优化

- **动态内存管理**：按需分配内存，支持灵活的按键数量
- **最小化GPIO操作**：优化GPIO读取频率
- **高效状态切换**：优化的状态机转换逻辑

## 注意事项

1. **定时更新**：必须定期调用`key_manager_update`函数（建议5ms间隔）
2. **内存管理**：使用后必须调用`key_manager_destroy`释放内存
3. **GPIO配置**：确保GPIO引脚未被其他模块占用
4. **中断安全**：模块非中断安全，需在任务上下文中使用

## 相关文件

- `key_manager.h` - 头文件，包含API声明和类型定义
- `key_manager.c` - 实现文件，包含完整功能实现

[返回主文档](../../../../../../../../README.md)