/**
 ****************************************************************************************************
 * @file        bsp_dht11.h
 * @author      普中科技
 * @version     V1.0
 * @date        2024-06-05
 * @brief       DHT11温湿度传感器实验
 * @license     Copyright (c) 2024-2034, 深圳市普中科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:普中-Hi3861
 * 在线视频:https://space.bilibili.com/2146492485
 * 公司网址:www.prechin.cn
 * 购买地址:
 *
 */

#ifndef DHT11_H
#define DHT11_H

#include "cmsis_os2.h"
#include "hi_io.h"
#include "hi_gpio.h"

#include "debug.h"   // 添加debug库支持
#include "pin_definitions.h"

#define DHT11_DQ_OUT(a)   hi_gpio_set_ouput_val(DHT11_PIN,a)

//函数声明
void dht11_io_out(void);
void dht11_io_in(void);
void dht11_reset(void);
uint8_t dht11_check(void);
uint8_t dht11_read_bit(void);
uint8_t dht11_read_byte(void);
uint8_t dht11_read_data(uint8_t *temp,uint8_t *humi) ;
uint8_t dht11_init(void);

#endif