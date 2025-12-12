#ifndef __DHT11_H__
#define __DHT11_H__

#include <stdint.h>
#include <stdbool.h>

// DHT11引脚定义 - 使用IO1
#define DHT11_PIN         HI_IO_NAME_GPIO_1
#define DHT11_GPIO_FUN    HI_IO_FUNC_GPIO_1_GPIO

// DHT11温湿度传感器函数
bool DHT11_Init(void);
bool DHT11_ReadData(uint8_t* humidity, uint8_t* temperature);
bool DHT11_IsConnected(void);

#endif // __DHT11_H__