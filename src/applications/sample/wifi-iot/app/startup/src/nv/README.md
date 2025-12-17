# NV存储模块

## 模块概述

NV（Non-Volatile）存储模块提供键值对（KV）数据持久化存储功能，支持在Flash中保存配置数据、用户设置等需要持久化的信息。

## 功能特性

- ✅ 键值对数据存储
- ✅ CRC数据校验
- ✅ 自动保存机制
- ✅ 数据压缩和索引
- ✅ 存储状态监控

## 存储结构

### KV对定义
```c
typedef struct {
    char key[NV_MAX_KEY_LEN];          // 键（最大32字节）
    uint8_t value[NV_MAX_VALUE_LEN];   // 值（最大128字节）
    uint16_t value_len;                // 值长度
    uint32_t timestamp;                // 时间戳
    uint16_t crc;                      // CRC校验
} nv_kv_pair_t;
```

### 配置参数
```c
typedef struct {
    uint32_t base_addr;                // 存储基地址
    uint32_t total_size;               // 总大小
    uint32_t block_size;               // 块大小
    bool auto_save;                    // 自动保存
    uint32_t auto_save_interval;       // 保存间隔
} nv_config_t;
```

## API接口

### 初始化管理
```c
nv_result_t nv_init(const nv_config_t* config);
void nv_deinit(void);
```
初始化和反初始化NV库。

### 数据操作
```c
nv_result_t nv_set(const char* key, const void* value, uint16_t value_len);
nv_result_t nv_get(const char* key, void* value, uint16_t* value_len);
nv_result_t nv_delete(const char* key);
```
设置、获取和删除KV对。

### 查询功能
```c
bool nv_exists(const char* key);
nv_result_t nv_get_keys(char keys[][NV_MAX_KEY_LEN], uint16_t max_keys, uint16_t* key_count);
nv_result_t nv_get_count(uint16_t* count);
```
键存在性检查和列表查询。

### 存储管理
```c
nv_result_t nv_clear_all(void);
nv_result_t nv_save_to_flash(void);
nv_result_t nv_load_from_flash(void);
nv_result_t nv_get_status(uint32_t* used_size, uint32_t* free_size, uint32_t* total_size);
```
存储空间管理和Flash操作。

## 使用示例

### 基本配置
```c
nv_config_t config = {
    .base_addr = 0xA000,
    .total_size = 0x2000,
    .block_size = 0x1000,
    .auto_save = true,
    .auto_save_interval = 10000,
};

nv_result_t ret = nv_init(&config);
```

### 数据存储
```c
// 存储整数
int32_t temperature = 25;
nv_set("temp", &temperature, sizeof(temperature));

// 存储字符串
const char* name = "Device001";
nv_set("device_name", name, strlen(name) + 1);

// 存储浮点数
float humidity = 45.6f;
nv_set("humi", &humidity, sizeof(humidity));
```

### 数据读取
```c
// 读取整数
int32_t read_temp;
uint16_t len = sizeof(read_temp);
if (nv_get("temp", &read_temp, &len) == NV_SUCCESS) {
    log_i("NV", "Temperature: %d", read_temp);
}

// 读取字符串
char read_name[32];
len = sizeof(read_name);
if (nv_get("device_name", read_name, &len) == NV_SUCCESS) {
    log_i("NV", "Device name: %s", read_name);
}
```

## 内部实现

### 数据存储结构
[KV对1] -> [KV对2] -> ... -> [KV对N]
每个KV对包含：键、值、长度、时间戳、CRC