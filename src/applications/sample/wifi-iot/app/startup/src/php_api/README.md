# PHP API 模块使用说明

## 概述
PHP API模块是一个轻量级的网络数据发送库，专门为Hi3861 WiFi IoT设备设计。该模块移除了模块内任务创建，仅提供API调用上传接口。

## 主要特性
- ✅ 无模块内任务创建
- ✅ 单次调用单次发送
- ✅ WiFi连接后自动发送握手信息
- ✅ 支持自定义JSON数据发送
- ✅ 支持传感器数据和设备状态数据发送
- ✅ 配置信息持久化存储

## API接口说明

### 初始化
```c
php_api_result_t php_api_init(void);
```

### 核心接口
```c
// 发送握手信息（包含IP、设备名称）
php_api_result_t php_api_send_handshake(void);

// 发送JSON数据到服务器
php_api_result_t php_api_send_json_data(const char *json_data);

// 发送传感器数据
php_api_result_t php_api_send_sensor_data(void);

// 发送设备状态数据
php_api_result_t php_api_send_status_data(void);
```

### 辅助接口
```c
// 获取系统状态信息
php_api_result_t php_api_get_system_status(php_api_system_status_t *status);

// 获取传感器数据
php_api_result_t php_api_get_sensor_data(php_api_sensor_data_t *sensor_data);

// 设置/获取服务器IP
php_api_result_t php_api_set_server_ip(const char *server_ip);
php_api_result_t php_api_get_server_ip(char *server_ip, uint32_t buffer_size);
```

## 使用示例

### 基本使用
```c
// 初始化模块
php_api_init();

// WiFi连接成功后发送握手信息
if (WiFi_IsConnected()) {
    php_api_send_handshake();
}

// 发送传感器数据
php_api_send_sensor_data();

// 发送自定义JSON数据
const char *json = "{\"type\":\"custom\",\"data\":\"hello\"}";
php_api_send_json_data(json);
```

### 在main.c中的使用
```c
// 网络任务中初始化
php_api_result_t ret = php_api_init();
if (ret == PHP_API_SUCCESS) {
    // WiFi连接后发送握手信息
    if (g_wifi_connected) {
        php_api_send_handshake();
    }
}

// 定时发送数据
if (current_time_ms - last_send_time >= 30000) { // 30秒间隔
    php_api_send_sensor_data();
}
```

## 配置说明

### 服务器IP配置
服务器IP地址通过KV存储持久化保存：
- 默认IP: `192.168.1.100`
- 可以通过`php_api_set_server_ip()`函数设置
- 配置保存在KV存储的`server_ip`键中

### 设备信息
- 设备名称: `Hi3861_IoT_Device`

## 错误处理
所有API函数都返回`php_api_result_t`枚举值，包含详细的错误信息：
- `PHP_API_SUCCESS`: 操作成功
- `PHP_API_ERROR_WIFI_NOT_CONNECTED`: WiFi未连接
- `PHP_API_ERROR_SERVER_IP_NOT_SET`: 服务器IP未设置
- 等等...

## 注意事项
1. 确保在调用发送函数前WiFi已连接
2. 服务器IP需要正确设置
3. 传感器数据需要从全局变量获取
4. 模块不创建任何内部任务，完全由外部调用控制