<div align="center">

# Hi3861 WiFi-IoT 智能设备项目

<!-- 语言切换按钮 -->
<div>
  <a href="README.md" style="background-color: #007bff; color: white; border: none; padding: 8px 16px; margin: 5px; border-radius: 4px; cursor: pointer; text-decoration: none;">中文</a>
  <a href="README_en.md" style="background-color: #6c757d; color: white; border: none; padding: 8px 16px; margin: 5px; border-radius: 4px; cursor: pointer; text-decoration: none;">English</a>
</div>

</div>

## 📖 项目简介

本项目是基于华为Hi3861 WiFi芯片的OpenHarmony物联网应用，实现了一个完整的智能设备系统。项目集成了多种传感器、执行器和通信模块，展示了物联网设备的完整功能。

## 🚀 主要功能特性

### 🔄 最新更新
- ✅ **高级按键管理器模块** - 支持多按键动态管理，多种事件检测
- ✅ **标准KV存储系统** - 从自定义NV模块升级，解决兼容性问题
- ✅ **智能WiFi连接** - 支持AP/STA双模式，Web界面配置
- ✅ **中文Web界面** - 修复字符编码问题，支持中文正常显示

### 🎯 核心模块
- **蜂鸣器模块** - PWM控制，报警和提示音功能
- **WiFi网络模块** - 双模WiFi连接，智能配置
- **DHT11传感器模块** - 温湿度数据采集
- **时间管理模块** - 精确延时控制
- **KV存储模块** - Flash数据持久化
- **PHP API模块** - HTTP服务器和Web接口
- **按键管理器模块** - 高级按键事件检测

## 🛠️ 技术架构

### 硬件要求
- 普中Hi3861开发板
- DHT11温湿度传感器
- 蜂鸣器模块
- LED指示灯
- 按键开关（GPIO11、GPIO12）
- WiFi天线

### 软件架构

## 主要项目目录
- **[主程序入口](src/applications/sample/wifi-iot/app/startup/README.md)** - 项目的启动模块，负责系统初始化和任务调度

## 主要功能模块

- **[蜂鸣器模块](src/applications/sample/wifi-iot/app/startup/src/buzzer/README.md)** - 支持PWM控制的蜂鸣器，提供报警和提示音功能
- **[WiFi网络模块](src/applications/sample/wifi-iot/app/startup/src/network/README.md)** - 支持AP和STA模式的双模WiFi连接
- **[DHT11传感器模块](src/applications/sample/wifi-iot/app/startup/src/sensors/README.md)** - 温湿度传感器数据采集
- **[时间管理模块](src/applications/sample/wifi-iot/app/startup/src/time/README.md)** - 系统时间管理和精确延时控制
- **[KV存储模块](src/applications/sample/wifi-iot/app/startup/src/kv/README.md)** - 基于LiteOS KV存储系统的键值对存储和Flash数据持久化
- **[PHP API模块](src/applications/sample/wifi-iot/app/startup/src/php_api/README.md)** - HTTP服务器和Web接口，支持智能WiFi连接
- **[按键管理器模块](src/applications/sample/wifi-iot/app/startup/src/key_manager/README.md)** - 高级按键事件检测，支持短按、长按、双击等事件
- **[启动模块](src/applications/sample/wifi-iot/app/startup/README.md)** - 系统初始化和任务调度
- **[OLED显示模块](src/applications/sample/wifi-iot/app/startup/src/oled/)** - 图形化信息显示（待集成）
- **[字体库模块](src/applications/sample/wifi-iot/app/startup/src/fonts/)** - 中文字体支持（待集成）

## 技术更新日志

### 🔄 按键管理器集成 (最新)
- ✅ **新增高级按键管理器模块**
- ✅ 支持多按键动态管理，可配置消抖时间、长按时间、双击间隔
- ✅ 支持多种按键事件：按下、释放、短按、长按、双击、保持按下
- ✅ 集成到主程序定时器，5ms周期检测按键状态
- ✅ 按键1：短按切换LED，长按显示系统状态，双击LED闪烁
- ✅ 按键2：短按蜂鸣器提示音，长触发报警，双击显示按键状态

### 🔄 存储系统升级
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
- 按键开关（GPIO11、GPIO12）
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
- 按键管理器初始化，支持GPIO11和GPIO12按键检测
- HTTP服务器启动，可通过浏览器访问设备配置界面

### 5. 按键功能
- **按键1短按**：切换LED状态
- **按键1长按**：显示系统状态（WiFi、温度、湿度等）
- **按键1双击**：LED闪烁3次
- **按键2短按**：蜂鸣器提示音
- **按键2长按**：蜂鸣器报警
- **按键2双击**：显示按键状态信息

### 6. Web访问
- 打开浏览器访问 `http://192.168.0.1/`
- 查看设备状态和配置WiFi网络
- 中文界面正常显示，无乱码问题

## 按键管理器特性

### 核心功能
- **动态管理**：支持运行时添加/移除按键
- **多种事件**：支持短按、长按、双击等复杂事件检测
- **可配置参数**：消抖时间、长按时间、双击间隔均可配置
- **状态查询**：实时查询按键状态和按下持续时间

### API接口
```c
// 创建按键管理器
key_manager_t *key_mgr = key_manager_create(2);

// 添加按键配置
key_manager_add_key(key_mgr, HI_IO_NAME_GPIO_11, 0, 20, 1000, 300);

// 定期更新状态
key_manager_update(key_mgr);

// 获取按键事件
key_event_t event = key_manager_get_event(key_mgr, 0);
```

### 定时器集成
按键管理器已集成到主程序的5ms定时器中，无需手动调用更新函数，自动检测按键事件。

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
│ ├── nfc/ # NFC模块 
│ ├── php_api/ # PHP API模块 
│ ├── key_manager/# 按键管理器模块 
│ ├── oled/ # OLED显示模块（待集成） 
│ └── fonts/ # 字体库模块（待集成） 
├── main.c # 主程序入口 
├── debug.h # 调试头文件
└── BUILD.gn # 构建配置文件

```