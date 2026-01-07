#ifndef __PIN_DEFINITIONS_H__
#define __PIN_DEFINITIONS_H__

#include "hi_io.h"
#include "hi_gpio.h"

// =============================================
// 蜂鸣器引脚定义 (buzzer.h)
// =============================================
#define BEEP_PIN                    HI_IO_NAME_GPIO_14
#define BEEP_PIN_PWM_FUNC           HI_IO_FUNC_GPIO_14_PWM5_OUT
#define BEEP_PWM_PORT               HI_PWM_PORT_PWM5

// =============================================
// DHT11温湿度传感器引脚定义 (dht11.h)
// =============================================
#define DHT11_PIN                   HI_IO_NAME_GPIO_7
#define DHT11_GPIO_FUN              HI_IO_FUNC_GPIO_7_GPIO

// =============================================
// 烟雾传感器引脚定义 (smoke.h)
// =============================================
#define SMOKE_SENSOR_PIN            HI_IO_NAME_GPIO_11  // 使用ADC5通道

// =============================================
// 继电器引脚定义 (relay.h)
// =============================================
#define RELAY_GPIO_PIN              HI_IO_NAME_GPIO_8
#define RELAY_GPIO_IDX              HI_GPIO_IDX_8
#define RELAY_GPIO_FUNC             HI_IO_FUNC_GPIO_8_GPIO

// =============================================
// OLED显示屏引脚定义 (oled_driver.h)
// =============================================
#define OLED_SW_I2C_SCL_PIN         HI_IO_NAME_GPIO_9
#define OLED_SW_I2C_SDA_PIN         HI_IO_NAME_GPIO_10

// =============================================
// 引脚功能定义汇总
// =============================================

// GPIO引脚使用情况汇总
typedef enum {
    PIN_GPIO_7  = HI_IO_NAME_GPIO_7,   // DHT11温湿度传感器
    PIN_GPIO_8  = HI_IO_NAME_GPIO_8,   // 继电器控制
    PIN_GPIO_9  = HI_IO_NAME_GPIO_9,   // OLED SCL (软件I2C)
    PIN_GPIO_10 = HI_IO_NAME_GPIO_10,  // OLED SDA (软件I2C)
    PIN_GPIO_11 = HI_IO_NAME_GPIO_11,  // 烟雾传感器 (ADC5)
    PIN_GPIO_14 = HI_IO_NAME_GPIO_14   // 蜂鸣器 (PWM5)
} pin_gpio_usage_t;

// PWM引脚使用情况
typedef enum {
    PWM_PORT_5 = HI_PWM_PORT_PWM5     // 蜂鸣器使用PWM5
} pin_pwm_usage_t;

// ADC引脚使用情况
typedef enum {
    ADC_CHANNEL_5 = 5                 // 烟雾传感器使用ADC5
} pin_adc_usage_t;

// =============================================
// 引脚功能验证宏
// =============================================

// 检查引脚是否被占用
#define PIN_IS_USED(pin) \
    ((pin) == PIN_GPIO_7 || (pin) == PIN_GPIO_8 || (pin) == PIN_GPIO_9 || \
     (pin) == PIN_GPIO_10 || (pin) == PIN_GPIO_11 || (pin) == PIN_GPIO_14)

// 获取引脚用途描述
static inline const char* pin_get_usage_description(hi_io_name pin) {
    switch (pin) {
        case PIN_GPIO_7:  return "DHT11温湿度传感器";
        case PIN_GPIO_8:  return "继电器控制";
        case PIN_GPIO_9:  return "OLED SCL (软件I2C)";
        case PIN_GPIO_10: return "OLED SDA (软件I2C)";
        case PIN_GPIO_11: return "烟雾传感器 (ADC5)";
        case PIN_GPIO_14: return "蜂鸣器 (PWM5)";
        default: return "未定义用途";
    }
}

#endif // __PIN_DEFINITIONS_H__