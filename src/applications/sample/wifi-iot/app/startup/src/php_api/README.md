# PHP API模块

## 模块概述

PHP API模块提供基于HTTP协议的RESTful API接口，支持设备远程控制和状态监控。该模块内置轻量级HTTP服务器，提供Web配置界面和纯网络API两种访问方式，支持智能WiFi连接逻辑和UTF-8字符编码。

## 功能特性

- ✅ **HTTP服务器** - 内置轻量级HTTP服务器，支持多客户端连接
- ✅ **RESTful API** - 提供标准的RESTful API接口
- ✅ **智能WiFi连接** - 开机自动读取配置，智能连接WiFi网络
- ✅ **Web配置界面** - 提供Web界面用于设备配置
- ✅ **多模块集成** - 支持蜂鸣器、LED、传感器等模块控制
- ✅ **字符编码支持** - 修复网页中文乱码，支持UTF-8编码
- ✅ **可配置超时** - 支持通过配置修改扫描和连接超时时间

## 技术更新

- **版本**: v2.0（纯网络API）
- **变更**: 移除网页内嵌，专注设备控制API
- **特性**: 支持蜂鸣器控制、LED控制、实时温湿度数据获取

## API接口列表

### 1. 系统状态查询
- **路径**: `GET /api/status`
- **功能**: 获取系统整体状态信息
- **响应**: JSON格式的系统状态数据

### 2. 实时传感器数据
- **路径**: `GET /api/sensor/data`
- **功能**: 获取实时温湿度数据
- **响应**: JSON格式的传感器数据

### 3. 蜂鸣器控制
- **路径**: `POST /api/buzzer/control`
- **功能**: 控制蜂鸣器发声
- **参数**: 
  - `mode`: 控制模式（0:单次响, 1:报警模式）
  - `duration`: 持续时间（毫秒）
  - `on_time`: 开启时间（报警模式）
  - `off_time`: 关闭时间（报警模式）
  - `repeat`: 重复次数

### 4. LED控制
- **路径**: `POST /api/led/control`
- **功能**: 控制LED指示灯
- **参数**:
  - `state`: 状态（0:关闭, 1:开启, 2:闪烁）
  - `interval`: 闪烁间隔（毫秒）

### 5. 设备信息
- **路径**: `GET /api/device/info`
- **功能**: 获取设备基本信息
- **响应**: JSON格式的设备信息

### 6. WiFi配置接口
- **路径**: `POST /api/wifi/config`
- **功能**: 配置WiFi网络参数
- **参数**:
  - `ssid`: WiFi网络SSID
  - `password`: WiFi密码

### 7. Web界面接口
- **路径**: `GET /` 或 `GET /index.html`
- **功能**: 设备Web配置界面
- **响应**: HTML格式的配置页面

## 智能WiFi连接逻辑

### 连接流程
1. **开机初始化**: 读取KV存储中的`sta_ssid`和`sta_password`
2. **STA连接尝试**: 如果存在SSID配置，尝试连接对应网络
   - 扫描超时: 30秒（可通过配置修改）
   - 连接超时: 60秒（可通过配置修改）
3. **AP模式回退**: 如果连接失败或没有配置，开启无密码AP
4. **Web配置**: 通过浏览器访问Web界面配置网络参数

### 连接状态机
```c
typedef enum {
    WIFI_STATE_INIT,      // 初始化状态
    WIFI_STATE_SCANNING,  // 扫描中
    WIFI_STATE_CONNECTING,// 连接中
    WIFI_STATE_CONNECTED, // 已连接
    WIFI_STATE_AP_MODE,   // AP模式
    WIFI_STATE_ERROR      // 错误状态
} wifi_state_t;
```

## 字符编码修复

### 问题描述
原始版本存在网页中文乱码问题，主要原因是：
- HTTP响应头缺少字符集声明
- HTML页面字符编码设置不正确

### 修复方案
1. **HTTP头修复**: 在响应头中添加 `charset=utf-8` 参数
2. **HTML结构修复**: 添加正确的HTML文档结构和meta标签
3. **双重保障**: HTTP头和HTML meta标签双重字符编码声明

### 修复代码示例
```c
// 发送HTTP响应时添加字符集
php_api_send_response(client_socket, 200, "text/html; charset=utf-8", html);

// HTML页面添加meta标签
const char* html = 
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<meta charset=\"UTF-8\">"
    "<title>Hi3861配置界面</title>"
    "</head>"
    // ... 页面内容
    "</html>";
```

## 使用示例

### PHP客户端示例
```php
<?php
// 获取系统状态
$status = file_get_contents('http://192.168.1.1/api/status');
$data = json_decode($status, true);
echo "温度: " . $data['data']['temperature'] . "°C\n";

// 控制蜂鸣器
$post_data = json_encode([
    'mode' => 0,
    'duration' => 1000
]);
$context = stream_context_create([
    'http' => [
        'method' => 'POST',
        'header' => 'Content-Type: application/json',
        'content' => $post_data
    ]
]);
$result = file_get_contents('http://192.168.1.1/api/buzzer/control', false, $context);
```

### 命令行测试
```bash
# 获取系统状态
curl http://192.168.1.1/api/status

# 控制LED闪烁
curl -X POST http://192.168.1.1/api/led/control \
  -H "Content-Type: application/json" \
  -d '{"state":2,"interval":500}'

# 配置WiFi网络
curl -X POST http://192.168.1.1/api/wifi/config \
  -H "Content-Type: application/json" \
  -d '{"ssid":"MyWiFi","password":"mypassword"}'
```

### C语言集成示例
```c
#include "php_api.h"

// 初始化模块
php_api_result_t ret = php_api_init();
if (ret == PHP_API_SUCCESS) {
    printf("PHP API模块初始化成功\n");
}

// 启动智能WiFi连接
php_api_smart_wifi_connect();

// 设置超时时间
php_api_set_wifi_timeout(30000, 60000); // 30秒扫描，60秒连接

// 启动HTTP服务器
php_api_start_server();

// 在主循环中处理请求
while (1) {
    php_api_process_requests();
    osDelay(100);
}
```

## 内部实现

### HTTP服务器架构
```c
// HTTP请求处理状态机
typedef enum {
    HTTP_STATE_IDLE,      // 空闲状态
    HTTP_STATE_READING,   // 读取请求
    HTTP_STATE_PROCESSING,// 处理请求
    HTTP_STATE_WRITING,   // 写入响应
    HTTP_STATE_CLOSING    // 关闭连接
} http_state_t;

// 请求解析结构
typedef struct {
    char method[16];      // 请求方法
    char path[256];       // 请求路径
    char version[16];     // HTTP版本
    char headers[1024];   // 请求头
    char body[2048];      // 请求体
} http_request_t;
```

### API路由处理
```c
// API路由表
typedef struct {
    const char* path;     // 路径匹配
    http_method_t method; // 请求方法
    api_handler_t handler;// 处理函数
} api_route_t;

// 路由处理函数示例
api_result_t handle_status_api(http_request_t* req, http_response_t* resp) {
    // 构建状态JSON响应
    snprintf(resp->body, sizeof(resp->body), 
        "{\"status\":\"ok\",\"temperature\":%.1f,\"humidity\":%d}",
        get_temperature(), get_humidity());
    resp->content_type = "application/json";
    return API_SUCCESS;
}
```

## 配置参数

### KV存储键名
- `sta_ssid` - STA网络SSID
- `sta_password` - STA网络密码
- `scan_timeout` - 扫描超时时间（毫秒）
- `connect_timeout` - 连接超时时间（毫秒）

### 默认配置
- **服务器端口**: 80
- **最大客户端数**: 5
- **请求超时**: 30秒
- **字符编码**: UTF-8
- **缓冲区大小**: 2KB

## 性能优化

### 内存管理
- **静态分配**: 使用固定大小的缓冲区，避免动态分配
- **缓冲区复用**: 重用请求和响应缓冲区
- **连接池**: 限制最大连接数，防止内存耗尽

### 处理优化
- **非阻塞IO**: 使用非阻塞socket操作
- **请求解析优化**: 快速路径匹配和参数解析
- **响应缓存**: 缓存静态资源响应

## 安全考虑

### 输入验证
- **路径验证**: 防止路径遍历攻击
- **参数验证**: 验证API参数范围和格式
- **缓冲区保护**: 防止缓冲区溢出

### 访问控制
- **连接限制**: 限制最大并发连接数
- **超时控制**: 自动断开超时连接
- **错误处理**: 安全的错误信息返回

## 故障排除

### 常见问题
1. **连接失败**: 检查网络配置和端口占用
2. **中文乱码**: 验证字符编码设置
3. **API无响应**: 检查模块初始化和集成
4. **内存不足**: 调整缓冲区大小和连接数

### 调试信息
```c
// 启用调试输出
#define PHP_API_DEBUG 1

#if PHP_API_DEBUG
#define php_api_printf(...) printf(__VA_ARGS__)
#else
#define php_api_printf(...)
#endif
```

## 相关文件

- `php_api.h` - 模块头文件，包含API接口定义
- `php_api.c` - 模块实现文件，包含HTTP服务器和业务逻辑
- `README.md` - 模块说明文档

## 依赖模块

- `network` - WiFi网络功能
- `kv` - KV存储功能
- `time` - 时间管理功能
- `buzzer` - 蜂鸣器控制功能
- `sensors` - 传感器数据获取

## 测试验证

### Web界面测试
1. 设备启动后，连接设备WiFi热点
2. 浏览器访问 `http://192.168.0.1/`
3. 验证中文界面正常显示，无乱码

### API接口测试
```bash
# 测试系统状态接口
curl http://192.168.0.1/api/status

# 测试传感器数据接口  
curl http://192.168.0.1/api/sensor/data

# 测试蜂鸣器控制
curl -X POST http://192.168.0.1/api/buzzer/control \
  -H "Content-Type: application/json" \
  -d '{"mode":0,"duration":1000}'
```

## 扩展功能

### 未来计划
- 🔄 **HTTPS支持**: 添加TLS加密支持
- 🔄 **认证机制**: 添加API密钥认证
- 🔄 **数据持久化**: 支持历史数据存储
- 🔄 **WebSocket**: 实时数据推送支持

[返回主文档](../../../../../../../../README.md)