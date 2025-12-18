# Hi3861 WiFi-IoT 智能设备项目

## 项目简介

本项目是基于华为Hi3861 WiFi芯片的OpenHarmony物联网应用，实现了一个完整的智能设备系统。项目集成了多种传感器、执行器和通信模块，展示了物联网设备的完整功能。

## 主要项目目录
- **[主程序入口](src/applications/sample/wifi-iot/app/startup/README.md)** 

## 主要功能模块

- **[蜂鸣器模块](src/applications/sample/wifi-iot/app/startup/src/buzzer/README.md)** - 支持PWM控制的蜂鸣器，提供报警和提示音功能
- **[WiFi网络模块](src/applications/sample/wifi-iot/app/startup/src/network/README.md)** - 支持AP和STA模式的双模WiFi连接
- **[DHT11传感器模块](src/applications/sample/wifi-iot/app/startup/src/sensors/README.md)** - 温湿度传感器数据采集
- **[时间管理模块](src/applications/sample/wifi-iot/app/startup/src/time/README.md)** - 系统时间管理和精确延时控制
- **[KV存储模块](src/applications/sample/wifi-iot/app/startup/src/kv/README.md)** - 基于LiteOS KV存储系统的键值对存储和Flash数据持久化
- **[PHP API模块](src/applications/sample/wifi-iot/app/startup/src/php_api/README.md)** - HTTP服务器和Web接口，支持智能WiFi连接
- **[启动模块](src/applications/sample/wifi-iot/app/startup/README.md)** - 系统初始化和任务调度
- **[OLED显示模块](src/applications/sample/wifi-iot/app/startup/src/oled/)** - 图形化信息显示（待集成）
- **[字体库模块](src/applications/sample/wifi-iot/app/startup/src/fonts/)** - 中文字体支持（待集成）

## 技术更新日志

### 🔄 存储系统升级 (最新)
- ✅ **从自定义NV模块升级到标准KV存储系统**
- ✅ 使用Hi3861标准LiteOS KV存储系统，解决兼容性问题
- ✅ 封装了UtilsSetValue/UtilsGetValue等底层API为高层接口
- ✅ 支持断电保存验证和自动配置初始化
- ✅ 解决了0x80003016错误码问题

### 字符编码修复
- ✅ 修复网页中文乱码问题
- ✅ HTTP响应头添加UTF-8字符集声明
- ✅ HTML页面添加meta charset标签
- ✅ 支持中文Web界面正常显示

### 智能WiFi连接
- ✅ 开机自动读取KV存储配置连接WiFi
- ✅ AP模式回退机制
- ✅ 可配置超时时间
- ✅ Web界面远程配置

## 硬件要求

- 普中Hi3861开发板
- DHT11温湿度传感器
- 蜂鸣器模块
- LED指示灯
- WiFi天线

## 快速开始

### 1. 环境搭建
```bash
# 安装OpenHarmony开发环境
# 配置Hi3861编译工具链
```

### 2. 编译项目
```bash
# 在项目根目录执行
hb build -f
```

### 3. 烧录程序
```bash
# 使用HiBurn工具烧录编译后的固件
```

### 4. 运行效果
- 系统启动后LED灯开始闪烁
- 蜂鸣器发出启动提示音
- KV存储系统初始化并进行断电保存验证
- DHT11传感器开始采集温湿度数据
- WiFi模块初始化并准备连接
- HTTP服务器启动，可通过浏览器访问设备配置界面

### 5. Web访问
- 打开浏览器访问 `http://192.168.0.1/`
- 查看设备状态和配置WiFi网络
- 中文界面正常显示，无乱码问题

## KV存储系统特性

### 核心功能
- **高层封装**: 将LiteOS KV存储API封装为易用接口
- **断电保存**: 支持Flash数据持久化，断电后数据不丢失
- **自动验证**: 开机自动检测数据完整性
- **配置管理**: 支持WiFi配置、设备参数等关键数据存储

### API接口
```c
// 初始化KV存储系统
kv_result_t kv_init(void);

// 字符串操作
kv_result_t kv_set_string(const char *key, const char *value);
kv_result_t kv_get_string(const char *key, char *buffer, size_t buffer_size);

// 整型操作  
kv_result_t kv_set_uint32(const char *key, uint32_t value);
kv_result_t kv_get_uint32(const char *key, uint32_t *value);

// 删除操作
kv_result_t kv_delete(const char *key);
```

### 断电保存验证
系统启动时会自动进行KV存储的断电保存验证：
- 首次运行：设置初始测试数据
- 后续运行：验证数据是否成功保存
- 显示启动次数和关键数据状态

## 项目结构
```
src/applications/sample/wifi-iot/app/startup/
├── src/ # 模块源码目录
│ ├── buzzer/ # 蜂鸣器模块
│ ├── network/ # WiFi网络模块
│ ├── sensors/ # 传感器模块
│ ├── time/ # 时间管理模块
│ ├── kv/ # KV存储模块
│ ├── php_api/ # PHP API模块
│ ├── oled/ # OLED显示模块
│ └── fonts/ # 字体库模块
├── main.c # 主程序入口
├── debug.h # 输出调试日志
└── BUILD.gn # 构建配置文件
```