#ifndef __DS18B20_H__
#define __DS18B20_H__

#include <stdint.h>
#include <stdbool.h>

// DS18B20引脚定义 - 使用IO1
#define DS18B20_PIN         HI_IO_NAME_GPIO_1
#define DS18B20_GPIO_FUN    HI_IO_FUNC_GPIO_1_GPIO

// DS18B20温度传感器函数
bool DS18B20_Init(void);
float DS18B20_ReadTemperature(void);
bool DS18B20_IsPresent(void);

// 延时函数声明
void Sensor_DelayUs(uint32_t us);
void Sensor_DelayMs(uint32_t ms);

#endif // __DS18B20_H__