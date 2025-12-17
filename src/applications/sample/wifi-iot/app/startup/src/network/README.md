# WiFi网络模块

## 模块概述

WiFi网络模块提供完整的WiFi连接管理功能，支持AP（热点）模式和STA（站点）模式，可实现设备作为WiFi热点或连接到现有WiFi网络。

## 功能特性

- ✅ 双模支持：AP模式和STA模式
- ✅ 自动IP地址获取（DHCP）
- ✅ 连接状态监控和回调
- ✅ 完整的错误处理和重试机制
- ✅ 支持WPA/WPA2安全协议

## 硬件要求

- Hi3861 WiFi芯片
- 外置天线
- 稳定的电源供应

## API接口

### 热点创建
```c
WifiErrorCode WiFi_createHotspots(const char *ssid, const char *psk);
```
创建WiFi热点，设备作为AP使用。

### 热点连接
```c
WifiErrorCode WiFi_connectHotspots(const char *ssid, const char *psk);
```
连接到指定的WiFi热点。

### IP地址获取
```c
char* WiFi_GetLocalIP(void);
```
获取连接后的本地IP地址。

### STA信息管理
```c
WifiErrorCode WiFi_setSTAInfo(const char *ssid, const char *password, int security_type);
WifiErrorCode WiFi_connectToSTA(void);
```
预配置STA信息并连接。

### 完整流程
```c
WifiErrorCode WiFi_startAPAndConnectSTA(const char *ap_ssid, const char *ap_password, 
                                       const char *sta_ssid, const char *sta_password);
```
完整的AP+STA流程：先创建AP，再连接到STA。

## 使用示例

### 创建WiFi热点
```c
WifiErrorCode ret = WiFi_createHotspots("MyDevice", "12345678");
if (ret == WIFI_SUCCESS) {
    log_i("WIFI", "Hotspot created successfully");
}
```

### 连接WiFi网络
```c
WifiErrorCode ret = WiFi_connectHotspots("MyWiFi", "mypassword");
if (ret == WIFI_SUCCESS) {
    char* ip = WiFi_GetLocalIP();
    log_i("WIFI", "Connected! IP: %s", ip);
}
```

### AP+STA模式
```c
// 设备先作为AP，然后连接到路由器
ret = WiFi_startAPAndConnectSTA("DeviceAP", "ap123456", 
                                "HomeWiFi", "homepassword");
```

## 内部实现

### 连接状态机
模块通过回调函数监控连接状态：
- `WIFI_STATE_AVALIABLE` - 连接可用
- 连接丢失处理

### DHCP流程
1. 启动DHCP客户端
2. 等待IP地址分配
3. 获取并存储IP地址

### 错误处理层级
- 硬件层错误（芯片初始化）
- 协议层错误（连接失败）
- 应用层错误（参数无效）

## 配置参数

### 超时设置
```c
#define DEF_TIMEOUT 15  // 默认超时时间（秒）
```

### WiFi通道
```c
#define WIFI_CHANNE 5   // WiFi通道号
```

### 网络接口
```c
#define SELECT_WLAN_PORT "wlan0"  // 网络接口名称
```

## 性能优化

### 连接优化
- 智能重试机制
- 超时检测和快速失败
- 连接状态缓存

### 资源管理
- 动态内存使用最小化
- 网络接口单例管理
- 回调函数注册机制

## 安全考虑

### 加密支持
- WPA-PSK加密
- WPA2-PSK加密
- 密码长度验证

### 网络安全
- SSID隐藏支持（可配置）
- MAC地址过滤（硬件支持）
- 连接数限制（AP模式）

## 故障排除

### 常见问题
1. **连接超时**：检查SSID和密码是否正确
2. **IP获取失败**：检查DHCP服务器状态
3. **热点创建失败**：检查硬件初始化

### 调试技巧
```c
// 启用详细日志
log_set_level(LOG_LEVEL_DEBUG);
```

## 相关文件

- `wifi.h` - 头文件，包含API声明和结构体定义
- `wifi.c` - 实现文件，包含完整网络功能

[返回主文档](../../../../../../../../README.md)