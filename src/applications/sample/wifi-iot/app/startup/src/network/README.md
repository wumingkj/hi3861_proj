# WiFi网络模块

## 模块概述

WiFi网络模块提供完整的WiFi连接管理功能，支持AP（热点）模式和STA（站点）模式，可实现设备作为WiFi热点或连接到现有WiFi网络。

## 技术更新

### 配置存储升级
- ✅ **从NV存储升级到KV存储系统**
- ✅ WiFi配置现在存储在标准的KV存储系统中
- ✅ 支持断电保存和自动配置恢复
- ✅ 提高配置管理的可靠性和兼容性

## 功能特性

- ✅ 双模支持：AP模式和STA模式
- ✅ 自动IP地址获取（DHCP）
- ✅ 连接状态监控和回调
- ✅ 完整的错误处理和重试机制
- ✅ 支持WPA/WPA2安全协议
- ✅ **KV存储集成**：WiFi配置持久化存储

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

## KV存储集成

### WiFi配置管理
网络模块现在使用KV存储系统管理WiFi配置：

```c
// 从KV存储读取WiFi配置
char wifi_ssid[32] = "";
char wifi_password[64] = "";

if (kv_get_string("wifi_ssid", wifi_ssid, sizeof(wifi_ssid)) == KV_SUCCESS && 
    kv_get_string("wifi_password", wifi_password, sizeof(wifi_password)) == KV_SUCCESS) {
    
    log_i("NETWORK", "从KV存储读取到WiFi配置: %s", wifi_ssid);
    
    // 尝试连接STA网络
    WifiErrorCode wifi_ret = WiFi_connectHotspots(wifi_ssid, wifi_password);
    
    if (wifi_ret == WIFI_SUCCESS) {
        log_i("NETWORK", "WiFi连接成功");
        g_wifi_connected = true;
    }
}
```

### 配置持久化
WiFi配置在KV存储中持久化保存：
- **wifi_ssid**: WiFi网络名称
- **wifi_password**: WiFi密码
- **scan_timeout**: 扫描超时时间（默认30秒）
- **connect_timeout**: 连接超时时间（默认60秒）

## 使用示例

### 创建WiFi热点
```c
WifiErrorCode ret = WiFi_createHotspots("MyDevice", "12345678");
if (ret == WIFI_SUCCESS) {
    log_i("WIFI", "Hotspot created successfully");
}
```

### 连接WiFi网络（使用KV存储）
```c
// 从KV存储读取配置并连接
char ssid[32], password[64];
if (kv_get_string("wifi_ssid", ssid, sizeof(ssid)) == KV_SUCCESS &&
    kv_get_string("wifi_password", password, sizeof(password)) == KV_SUCCESS) {
    
    WifiErrorCode ret = WiFi_connectHotspots(ssid, password);
    if (ret == WIFI_SUCCESS) {
        char* ip = WiFi_GetLocalIP();
        log_i("WIFI", "Connected! IP: %s", ip);
    }
}
```

### AP+STA模式（智能回退）
```c
// 先尝试STA连接，失败后回退到AP模式
if (!g_wifi_connected) {
    log_i("NETWORK", "启动AP模式作为备用方案");
    const char *ap_ssid = "Hi3861_Config_AP";
    const char *ap_password = "12345678";
    
    WifiErrorCode ap_ret = WiFi_createHotspots(ap_ssid, ap_password);
    if (ap_ret == WIFI_SUCCESS) {
        log_i("NETWORK", "AP热点创建成功: %s", ap_ssid);
    }
}
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

### KV存储集成流程
1. **启动时**: 从KV存储读取WiFi配置
2. **连接时**: 使用KV存储的配置进行连接
3. **配置更新**: 通过Web界面更新KV存储中的配置
4. **持久化**: 配置自动保存到Flash

## 配置参数

### 超时设置（存储在KV中）
```c
// 默认值，可从KV存储读取或设置
uint32_t scan_timeout, connect_timeout;
if (kv_get_uint32("scan_timeout", &scan_timeout) != KV_SUCCESS) {
    kv_set_uint32("scan_timeout", 30000);  // 30秒
}
if (kv_get_uint32("connect_timeout", &connect_timeout) != KV_SUCCESS) {
    kv_set_uint32("connect_timeout", 60000);  // 60秒
}
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

### KV存储优化
- 配置读取缓存
- 减少不必要的Flash写入
- 批量配置更新

## 安全考虑

### 加密支持
- WPA-PSK加密
- WPA2-PSK加密
- 密码长度验证

### KV存储安全
- 敏感信息（密码）安全存储
- 配置数据完整性验证
- 访问权限控制

## 故障排除

### 常见问题
1. **连接超时**：检查SSID和密码是否正确存储在KV中
2. **IP获取失败**：检查DHCP服务器状态
3. **KV读取失败**：检查KV存储初始化状态

### 调试技巧
```c
// 启用详细日志
log_set_level(LOG_LEVEL_DEBUG);

// 检查KV存储状态
char test_buffer[32];
if (kv_get_string("wifi_ssid", test_buffer, sizeof(test_buffer)) == KV_SUCCESS) {
    log_d("KV", "WiFi配置读取成功: %s", test_buffer);
}
```

## 相关文件

- `wifi.h` - 头文件，包含API声明和结构体定义
- `wifi.c` - 实现文件，包含完整网络功能
- `kv.h` - KV存储模块头文件
- `kv.c` - KV存储模块实现

[返回主文档](../../../../../../../../README.md)