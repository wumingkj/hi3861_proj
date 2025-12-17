#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "hi_io.h"
#include "hi_gpio.h"
#include "buzzer.h"
#include "dht11.h"
//#include "oled.h"
#include "time.h"    // 使用新的时间库
#include "wifi.h"

//#define DEFAULT_DEBUG_LEVEL
#define DEBUG_ENABLE_COLOR
#include "debug.h"   // 新增调试头文件

// 函数原型声明
void led_init(void);

// 管脚定义
#define LED_PIN         HI_IO_NAME_GPIO_2
#define LED_GPIO_FUN    HI_IO_FUNC_GPIO_2_GPIO

// LED控制宏定义（修复函数名错误）
#define LED_ON()        hi_gpio_set_ouput_val(LED_PIN, HI_GPIO_VALUE0)
#define LED_OFF()       hi_gpio_set_ouput_val(LED_PIN, HI_GPIO_VALUE1)

static osTimerId_t g_buzzer_tick_timer = NULL;

// 全局变量用于存储传感器数据
uint8_t g_temperature = 255;  // 改为uint8_t类型
uint8_t g_humidity = 255;     // 改为uint8_t类型
static bool g_dht11_connected = true;

static uint8_t led_value = 0;

// 常量定义（毫秒）
#define SENSOR_UPDATE_INTERVAL_MS 1000    // 1秒读取一次
#define LED_UPDATE_INTERVAL_MS    100     // 100ms LED闪烁

// DHT11读取重试机制
#define DHT11_RETRY_COUNT 3            // 每次读取最多重试3次

static void BuzzerTickCb(void *arg) {
    (void)arg;
    Buzzer_Tick(Time_GetCurrentMs());
}

// 主任务 - 优化版本
static void Main_Task(void) {
    uint32_t current_time_ms;
    
    // 初始化时间戳
    uint32_t g_last_sensor_update = Time_GetCurrentMs();
    uint32_t g_last_led_update = Time_GetCurrentMs();
    uint32_t g_last_buzzer_timer = Time_GetCurrentMs();

    log_i("MAIN", "Main task started successfully");
    
    while (1) {
        current_time_ms = Time_GetCurrentMs();
        
        // 1. 传感器读取（1秒间隔，减少干扰）
        if (current_time_ms - g_last_sensor_update >= SENSOR_UPDATE_INTERVAL_MS) {
            g_last_sensor_update = current_time_ms;

            if (dht11_read_data(&g_temperature, &g_humidity) == 0) {
                log_i("DHT11", "Humidity=%d%%, Temperature=%d°C", g_humidity, g_temperature);
            } else {
                log_e("DHT11", "Read failed");
            }
        }
        
        // 2. LED显示更新（100ms间隔）- 非阻塞延时方式
        if (current_time_ms - g_last_led_update >= LED_UPDATE_INTERVAL_MS) {
            g_last_led_update = current_time_ms;
            
            // LED闪烁控制
            if (led_value) {
                LED_ON();
                log_d("LED", "LED ON");
            } else {
                LED_OFF();
                log_d("LED", "LED OFF");
            }
            led_value = !led_value;
        }
        
        // 添加延时，让出CPU时间，防止看门狗超时
        Time_DelayMs(10);
    }
}

static void Main_Entry(void) {
    // 初始化库
    Time_Init();
    led_init();
    Buzzer_Init();

    log_i("SYSTEM", "System initialization started");

    // 初始化DHT11
    uint8_t retry_count = 0;
    while (dht11_init()) {
        log_w("DHT11", "Init failed, retrying... (attempt %d/%d)", retry_count + 1, DHT11_RETRY_COUNT);
        retry_count++;
        Time_DelayMsPrecise(1000); // 等待1秒后重试
        if (retry_count >= DHT11_RETRY_COUNT) {
            log_e("DHT11", "Init failed after %d attempts", DHT11_RETRY_COUNT);
            g_dht11_connected = false;
            break;
        }
    }
    
    if (g_dht11_connected) {
        log_i("DHT11", "Init success");
    }

    // ✅ main 里创建蜂鸣器tick定时器（建议 1~5ms）
    g_buzzer_tick_timer = osTimerNew(BuzzerTickCb, osTimerPeriodic, NULL, NULL);
    if (g_buzzer_tick_timer != NULL) {
        osTimerStart(g_buzzer_tick_timer, 5); // 5ms 一次就很够用了
        log_i("BUZZER", "Buzzer timer started successfully");
    } else {
        log_e("BUZZER", "Failed to create buzzer timer");
    }

    // 启动提示音（非阻塞）
    Buzzer_Alarm(2, 1000, 100);
    log_i("BUZZER", "Startup alarm triggered");
    
    log_i("SYSTEM", "System initialization completed successfully");
    
    // 创建主任务
    osThreadAttr_t main_attr = {
        .name = "MainTask",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U,
        .stack_mem = NULL,
        .stack_size = 8192,
        .priority = osPriorityNormal
    };
    
    if (osThreadNew((osThreadFunc_t)Main_Task, NULL, &main_attr) == NULL) {
        log_e("SYSTEM", "Failed to create Main_Task!");
    } else {
        log_i("SYSTEM", "Main task created successfully");
    }
}

APP_FEATURE_INIT(Main_Entry);

void led_init(void) {
    hi_gpio_init();                                            // GPIO初始化
    hi_io_set_pull(LED_PIN, HI_IO_PULL_DOWN);                  // 设置GPIO下拉
    hi_io_set_func(LED_PIN, LED_GPIO_FUN);                     // 设置IO为GPIO功能
    hi_gpio_set_dir(LED_PIN, HI_GPIO_DIR_OUT);                 // 设置GPIO为输出模式
    log_i("LED", "LED initialization completed");
}