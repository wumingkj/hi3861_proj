# KV存储模块

## 模块概述

KV存储模块是基于Hi3861标准LiteOS KV存储系统的高层封装，提供简单易用的键值对存储功能，支持Flash数据持久化。

## 技术背景

### 从NV模块升级到KV存储系统
- **问题解决**: 解决了自定义NV模块与Hi3861标准系统不兼容导致的0x80003016错误码问题
- **标准化**: 使用符合Hi3861规范的LiteOS KV存储系统
- **稳定性**: 避免了与标准系统的冲突，提高系统稳定性

### 底层依赖
- **LiteOS KV存储**: 基于`UtilsSetValue`/`UtilsGetValue`等标准API
- **Flash存储**: 支持断电数据保存
- **标准分区**: 使用Hi3861标准Flash分区配置

## 功能特性

### 核心功能
- ✅ **键值对存储**: 支持字符串和整型数据的存储与读取
- ✅ **断电保存**: Flash数据持久化，断电后数据不丢失
- ✅ **自动验证**: 开机自动进行数据完整性验证
- ✅ **配置管理**: 支持WiFi配置、设备参数等关键数据存储
- ✅ **错误处理**: 完善的错误码和状态返回机制

### 性能指标
- **键名长度**: 最大32字节
- **值长度**: 最大128字节
- **存储数量**: 最多50个键值对
- **响应时间**: 毫秒级读写速度

## API接口

### 初始化函数
```c
/**
 * @brief 初始化KV存储系统
 * @return 操作结果状态码
 */
kv_result_t kv_init(void);
```

### 字符串操作
```c
/**
 * @brief 设置字符串键值对
 * @param key 键名
 * @param value 字符串值
 * @return 操作结果状态码
 */
kv_result_t kv_set_string(const char *key, const char *value);

/**
 * @brief 获取字符串值
 * @param key 键名
 * @param buffer 存储结果的缓冲区
 * @param buffer_size 缓冲区大小
 * @return 操作结果状态码
 */
kv_result_t kv_get_string(const char *key, char *buffer, size_t buffer_size);
```

### 整型操作
```c
/**
 * @brief 设置32位无符号整型值
 * @param key 键名
 * @param value 整型值
 * @return 操作结果状态码
 */
kv_result_t kv_set_uint32(const char *key, uint32_t value);

/**
 * @brief 获取32位无符号整型值
 * @param key 键名
 * @param value 存储结果的指针
 * @return 操作结果状态码
 */
kv_result_t kv_get_uint32(const char *key, uint32_t *value);
```

### 删除操作
```c
/**
 * @brief 删除指定的键值对
 * @param key 要删除的键名
 * @return 操作结果状态码
 */
kv_result_t kv_delete(const char *key);
```

## 返回值定义

```c
typedef enum {
    KV_SUCCESS = 0,                    // 操作成功
    KV_ERROR_INIT_FAILED = -1,         // 初始化失败
    KV_ERROR_KEY_TOO_LONG = -2,        // 键名过长
    KV_ERROR_VALUE_TOO_LONG = -3,      // 值过长
    KV_ERROR_BUFFER_TOO_SMALL = -4,    // 缓冲区太小
    KV_ERROR_KEY_NOT_FOUND = -5,       // 键不存在
    KV_ERROR_STORAGE_FULL = -6,        // 存储空间已满
    KV_ERROR_INVALID_PARAM = -7,       // 参数无效
    KV_ERROR_SYSTEM = -8               // 系统错误
} kv_result_t;
```

## 使用示例

### 基本使用
```c
#include "kv.h"

// 初始化KV存储
if (kv_init() == KV_SUCCESS) {
    printf("KV存储初始化成功\n");
}

// 存储WiFi配置
kv_set_string("wifi_ssid", "MyWiFi");
kv_set_string("wifi_password", "12345678");

// 读取WiFi配置
char ssid[32], password[64];
if (kv_get_string("wifi_ssid", ssid, sizeof(ssid)) == KV_SUCCESS) {
    printf("WiFi SSID: %s\n", ssid);
}
```

### 断电保存验证
```c
// 检查启动计数
char boot_count[16];
if (kv_get_string("boot_count", boot_count, sizeof(boot_count)) == KV_ERROR_KEY_NOT_FOUND) {
    // 第一次运行，设置初始值
    kv_set_string("boot_count", "1");
    kv_set_string("first_boot", "true");
} else {
    // 后续运行，验证数据保存
    int count = atoi(boot_count);
    count++;
    char new_count[16];
    snprintf(new_count, sizeof(new_count), "%d", count);
    kv_set_string("boot_count", new_count);
    printf("第%d次启动，KV数据保存验证成功\n", count);
}
```

## 集成到主程序

### 在main.c中的使用
```c
#include "kv.h"

static void Main_Entry(void) {
    // 初始化KV存储
    kv_result_t kv_ret = kv_init();
    if (kv_ret == KV_SUCCESS) {
        printf("KV存储模块初始化成功\n");
        
        // 进行断电保存验证
        // ... 验证逻辑
    }
    
    // 其他系统初始化...
}
```

### 在BUILD.gn中的配置
```gn
static_library("startup") {
    sources = [
        "main.c",
        "src/kv/kv.c",  # KV存储模块
        # ... 其他源文件
    ]
    
    include_dirs = [
        "src/kv",  # KV模块头文件路径
        # ... 其他包含路径
    ]
}
```

## 故障排除

### 常见问题

1. **KV存储初始化失败**
   - 检查Flash分区配置是否正确
   - 确认系统资源是否充足

2. **数据读取失败**
   - 确认键名是否正确
   - 检查缓冲区大小是否足够

3. **断电保存验证失败**
   - 检查Flash硬件是否正常
   - 确认电源稳定性

### 调试信息
系统启动时会输出详细的KV存储状态信息：
- 初始化结果和错误码
- 断电保存验证状态
- 关键数据读取结果

## 版本历史

### v1.0 (最新)
- ✅ 从自定义NV模块升级到标准KV存储系统
- ✅ 封装LiteOS KV存储API为高层接口
- ✅ 实现断电保存验证功能
- ✅ 解决0x80003016兼容性问题

### 未来计划
- 🔄 支持更多数据类型（浮点数、结构体等）
- 🔄 增加数据备份和恢复功能
- 🔄 优化存储空间利用率

## 相关文件

- **kv.h**: 头文件，包含API声明和类型定义
- **kv.c**: 实现文件，包含API的具体实现
- **main.c**: 主程序，展示KV存储的使用示例

## 许可证
本模块基于Apache 2.0许可证开源。

[返回主文档](../../../../../../../../README.md)