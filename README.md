# Hi3861 WiFi-IoT 智能设备项目

## 项目简介

本项目是基于华为Hi3861 WiFi芯片的OpenHarmony物联网应用，实现了一个完整的智能设备系统。项目集成了多种传感器、执行器和通信模块，展示了物联网设备的完整功能。

## 主要功能模块

- **[蜂鸣器模块](src/applications/sample/wifi-iot/app/startup/src/buzzer/README.md)** - 支持PWM控制的蜂鸣器，提供报警和提示音功能
- **[WiFi网络模块](src/applications/sample/wifi-iot/app/startup/src/network/README.md)** - 支持AP和STA模式的双模WiFi连接
- **[DHT11传感器模块](src/applications/sample/wifi-iot/app/startup/src/sensors/README.md)** - 温湿度传感器数据采集
- **[时间管理模块](src/applications/sample/wifi-iot/app/startup/src/time/README.md)** - 系统时间管理和精确延时控制
- **[NV存储模块](src/applications/sample/wifi-iot/app/startup/src/nv/README.md)** - 键值对存储和Flash数据持久化
- **[启动模块](src/applications/sample/wifi-iot/app/startup/README.md)** - 系统初始化和任务调度
- **[OLED显示模块](src/applications/sample/wifi-iot/app/startup/src/oled/)** - 图形化信息显示（待集成）
- **[字体库模块](src/applications/sample/wifi-iot/app/startup/src/fonts/)** - 中文字体支持（待集成）

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
- DHT11传感器开始采集温湿度数据
- WiFi模块初始化并准备连接

## 项目结构
src/applications/sample/wifi-iot/app/startup/
├── src/ # 模块源码目录
│ ├── buzzer/ # 蜂鸣器模块
│ ├── network/ # WiFi网络模块
│ ├── sensors/ # 传感器模块
│ ├── time/ # 时间管理模块
│ ├── nv/ # NV存储模块
│ ├── oled/ # OLED显示模块
│ └── fonts/ # 字体库模块
├── main.c # 主程序入口
└── BUILD.gn # 构建配置文件