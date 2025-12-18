# PHP API模块

PHP API模块提供HTTP服务器功能和Web接口，支持智能WiFi连接逻辑和远程配置。

## 功能特性

- **HTTP服务器**: 内置轻量级HTTP服务器，支持多客户端连接
- **RESTful API**: 提供标准的RESTful API接口
- **智能WiFi连接**: 开机自动读取NV配置，智能连接WiFi网络
- **Web配置界面**: 提供Web界面用于设备配置
- **可配置超时**: 支持通过NV配置扫描和连接超时时间
- **PHP客户端支持**: 提供通用PHP客户端API接口
- **字符编码支持**: 修复网页中文乱码，支持UTF-8编码

## API接口

### 系统状态接口
- `GET /api/status` - 获取系统状态信息
- 返回JSON格式的系统状态数据

### WiFi配置接口  
- `POST /api/wifi/config` - 配置WiFi网络参数
- 支持STA SSID和密码配置

### 传感器数据接口
- `GET /api/sensor/data` - 获取传感器数据
- 返回温湿度等传感器数据

### Web界面接口
- `GET /` 或 `GET /index.html` - 设备Web配置界面
- 支持中文显示，无乱码问题

## 智能WiFi连接逻辑

1. **开机初始化**: 读取NV中的`sta_ssid`和`sta_password`
2. **STA连接尝试**: 如果存在SSID配置，尝试连接对应网络
   - 扫描超时: 30秒（可通过NV修改）
   - 连接超时: 60秒（可通过NV修改）
3. **AP模式回退**: 如果连接失败或没有配置，开启无密码AP
4. **Web配置**: 通过PHP客户端访问Web界面配置网络参数

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

## 使用方法

### 初始化模块
```c
php_api_result_t ret = php_api_init();
if (ret == PHP_API_SUCCESS) {
    // 初始化成功
}
```

### 启动智能WiFi连接
```c
php_api_smart_wifi_connect();
```

### 设置超时时间
```c
php_api_set_wifi_timeout(30000, 60000); // 30秒扫描，60秒连接
```

### 启动HTTP服务器
```c
php_api_start_server();
```

## 文件说明

- `php_api.h` - 模块头文件，包含API接口定义
- `php_api.c` - 模块实现文件，包含HTTP服务器和业务逻辑
- `README.md` - 模块说明文档

## 依赖模块

- `wifi` - WiFi网络功能
- `nv` - NV存储功能  
- `time` - 时间管理功能
- `debug` - 调试日志功能

## 配置参数

### NV存储键名
- `sta_ssid` - STA网络SSID
- `sta_password` - STA网络密码
- `scan_timeout` - 扫描超时时间（毫秒）
- `connect_timeout` - 连接超时时间（毫秒）

### 默认配置
- 服务器端口: 80
- 最大客户端数: 5
- 请求超时: 30秒
- 字符编码: UTF-8

## 示例代码

参考`main.c`中的使用示例，集成PHP API模块到主程序中。

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
```

[返回主文档](../../../../../../../../README.md)