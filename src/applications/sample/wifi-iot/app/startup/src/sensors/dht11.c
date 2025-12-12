#include "dht11.h"
#include "hi_io.h"
#include "hi_gpio.h"
#include <unistd.h>
#include <stdio.h>

// DHT11初始化
bool DHT11_Init(void)
{
    // 配置IO为GPIO功能
    hi_io_set_pull(DHT11_PIN, HI_IO_PULL_UP);
    hi_io_set_func(DHT11_PIN, DHT11_GPIO_FUN);
    hi_gpio_set_dir(DHT11_PIN, HI_GPIO_DIR_OUT);
    hi_gpio_set_ouput_val(DHT11_PIN, HI_GPIO_VALUE1); // 初始高电平
    
    printf("DHT11 Initialized on GPIO%d\n", DHT11_PIN);
    return true;
}

// 启动DHT11通信
static bool DHT11_Start(void)
{
    // 主机拉低总线至少18ms
    hi_gpio_set_dir(DHT11_PIN, HI_GPIO_DIR_OUT);
    hi_gpio_set_ouput_val(DHT11_PIN, HI_GPIO_VALUE0);
    usleep(20000); // 20ms
    
    // 主机释放总线，等待20-40us
    hi_gpio_set_dir(DHT11_PIN, HI_GPIO_DIR_IN);
    usleep(30); // 30us
    
    // 检查DHT11响应（80us低电平）
    hi_gpio_value val;
    hi_gpio_get_input_val(DHT11_PIN, &val);
    if (val != HI_GPIO_VALUE0) {
        return false;
    }
    
    // 等待80us低电平结束
    uint32_t timeout = 1000; // 超时计数
    while (val == HI_GPIO_VALUE0) {
        hi_gpio_get_input_val(DHT11_PIN, &val);
        if (--timeout == 0) return false;
        usleep(1); // 1us
    }
    
    // 检查80us高电平
    timeout = 1000;
    while (val == HI_GPIO_VALUE1) {
        hi_gpio_get_input_val(DHT11_PIN, &val);
        if (--timeout == 0) return false;
        usleep(1); // 1us
    }
    
    return true;
}

// 读取DHT11一个位
static uint8_t DHT11_ReadBit(void)
{
    // 等待50us低电平开始
    uint32_t timeout = 1000;
    hi_gpio_value val;
    
    while (1) {
        hi_gpio_get_input_val(DHT11_PIN, &val);
        if (val == HI_GPIO_VALUE1) break;
        if (--timeout == 0) return 0;
        usleep(1); // 1us
    }
    
    // 测量高电平持续时间
    uint32_t time_high = 0;
    timeout = 1000;
    
    while (val == HI_GPIO_VALUE1) {
        hi_gpio_get_input_val(DHT11_PIN, &val);
        time_high++;
        if (--timeout == 0) return 0;
        usleep(1); // 1us
    }
    
    // 高电平持续时间大于40us表示1，否则为0
    return (time_high > 40) ? 1 : 0;
}

// 读取DHT11一个字节
static uint8_t DHT11_ReadByte(void)
{
    uint8_t data = 0;
    
    for (int i = 0; i < 8; i++) {
        data <<= 1;
        data |= DHT11_ReadBit();
    }
    
    return data;
}

// 读取DHT11温湿度数据
bool DHT11_ReadData(uint8_t* humidity, uint8_t* temperature)
{
    if (!DHT11_Start()) {
        printf("DHT11 start failed\n");
        return false;
    }
    
    uint8_t data[5] = {0};
    
    // 读取40位数据
    for (int i = 0; i < 5; i++) {
        data[i] = DHT11_ReadByte();
    }
    
    // 校验和验证
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (data[4] != checksum) {
        printf("DHT11 checksum error: %d != %d\n", data[4], checksum);
        return false;
    }
    
    *humidity = data[0];        // 湿度整数部分
    *temperature = data[2];     // 温度整数部分
    
    printf("DHT11 Humidity: %d%%, Temperature: %d°C\n", *humidity, *temperature);
    return true;
}

// 检查DHT11是否连接
bool DHT11_IsConnected(void)
{
    hi_gpio_set_dir(DHT11_PIN, HI_GPIO_DIR_IN);
    hi_gpio_value val;
    hi_gpio_get_input_val(DHT11_PIN, &val);
    
    // DHT11数据线应该被上拉为高电平
    return (val == HI_GPIO_VALUE1);
}