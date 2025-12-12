#include "ds18b20.h"
#include "hi_io.h"
#include "hi_gpio.h"
#include <unistd.h>
#include <stdio.h>

// DS18B20命令定义
#define DS18B20_CMD_CONVERTTEMP    0x44
#define DS18B20_CMD_RSCRATCHPAD    0xBE
#define DS18B20_CMD_WSCRATCHPAD    0x4E
#define DS18B20_CMD_CPYSCRATCHPAD  0x48
#define DS18B20_CMD_RECEEPROM      0xB8
#define DS18B20_CMD_RPWRSUPPLY     0xB4
#define DS18B20_CMD_SEARCHROM      0xF0
#define DS18B20_CMD_READROM        0x33
#define DS18B20_CMD_MATCHROM       0x55
#define DS18B20_CMD_SKIPROM        0xCC
#define DS18B20_CMD_ALARMSEARCH    0xEC

// DS18B20初始化
bool DS18B20_Init(void)
{
    // 配置IO为GPIO功能
    hi_io_set_pull(DS18B20_PIN, HI_IO_PULL_UP);
    hi_io_set_func(DS18B20_PIN, DS18B20_GPIO_FUN);
    hi_gpio_set_dir(DS18B20_PIN, HI_GPIO_DIR_OUT);
    
    printf("DS18B20 Initialized on GPIO1\n");
    return true;
}

// 复位DS18B20
static bool DS18B20_Reset(void)
{
    bool presence = false;
    
    // 拉低总线480-960us
    hi_gpio_set_dir(DS18B20_PIN, HI_GPIO_DIR_OUT);
    hi_gpio_set_ouput_val(DS18B20_PIN, HI_GPIO_VALUE0);
    Sensor_DelayUs(480);
    
    // 释放总线，等待15-60us
    hi_gpio_set_dir(DS18B20_PIN, HI_GPIO_DIR_IN);
    Sensor_DelayUs(60);
    
    // 检查存在脉冲
    hi_gpio_value val;
    hi_gpio_get_input_val(DS18B20_PIN, &val);
    if (val == HI_GPIO_VALUE0) {
        presence = true;
    }
    
    // 等待总线恢复
    Sensor_DelayUs(480);
    
    return presence;
}

// 写一个字节到DS18B20
static void DS18B20_WriteByte(uint8_t data)
{
    hi_gpio_set_dir(DS18B20_PIN, HI_GPIO_DIR_OUT);
    
    for (int i = 0; i < 8; i++) {
        // 写1位
        if (data & 0x01) {
            // 写1：拉低1us，然后释放
            hi_gpio_set_ouput_val(DS18B20_PIN, HI_GPIO_VALUE0);
            Sensor_DelayUs(1);
            hi_gpio_set_dir(DS18B20_PIN, HI_GPIO_DIR_IN);
            Sensor_DelayUs(60);
        } else {
            // 写0：拉低60us，然后释放
            hi_gpio_set_ouput_val(DS18B20_PIN, HI_GPIO_VALUE0);
            Sensor_DelayUs(60);
            hi_gpio_set_dir(DS18B20_PIN, HI_GPIO_DIR_IN);
            Sensor_DelayUs(1);
        }
        data >>= 1;
    }
}

// 从DS18B20读取一个字节
static uint8_t DS18B20_ReadByte(void)
{
    uint8_t data = 0;
    
    for (int i = 0; i < 8; i++) {
        // 拉低总线1us
        hi_gpio_set_dir(DS18B20_PIN, HI_GPIO_DIR_OUT);
        hi_gpio_set_ouput_val(DS18B20_PIN, HI_GPIO_VALUE0);
        Sensor_DelayUs(1);
        
        // 释放总线，读取数据
        hi_gpio_set_dir(DS18B20_PIN, HI_GPIO_DIR_IN);
        Sensor_DelayUs(14);
        
        hi_gpio_value val;
        hi_gpio_get_input_val(DS18B20_PIN, &val);
        data >>= 1;
        if (val == HI_GPIO_VALUE1) {
            data |= 0x80;
        }
        
        Sensor_DelayUs(45);
    }
    
    return data;
}

// 读取温度值
float DS18B20_ReadTemperature(void)
{
    if (!DS18B20_Reset()) {
        printf("DS18B20 not present\n");
        return -999.0f;
    }
    
    // 跳过ROM匹配
    DS18B20_WriteByte(DS18B20_CMD_SKIPROM);
    
    // 开始温度转换
    DS18B20_WriteByte(DS18B20_CMD_CONVERTTEMP);
    
    // 等待转换完成（最大750ms）
    for (int i = 0; i < 750; i++) {
        Sensor_DelayMs(1);
        hi_gpio_value val;
        hi_gpio_get_input_val(DS18B20_PIN, &val);
        if (val == HI_GPIO_VALUE1) {
            break;
        }
    }
    
    // 复位并读取数据
    if (!DS18B20_Reset()) {
        return -999.0f;
    }
    
    DS18B20_WriteByte(DS18B20_CMD_SKIPROM);
    DS18B20_WriteByte(DS18B20_CMD_RSCRATCHPAD);
    
    uint8_t temp_low = DS18B20_ReadByte();
    uint8_t temp_high = DS18B20_ReadByte();
    
    // 组合温度值
    int16_t temp_raw = (temp_high << 8) | temp_low;
    float temperature = temp_raw * 0.0625f;
    
    printf("DS18B20 Temperature: %.2f°C\n", temperature);
    return temperature;
}

// 检查传感器是否存在
bool DS18B20_IsPresent(void)
{
    return DS18B20_Reset();
}