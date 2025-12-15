#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "hi_io.h"
#include "hi_gpio.h"
#include "buzzer.h"  // 修改为使用src/buzzer目录下的蜂鸣器库
#include "dht11.h"

// 函数声明
void led_init(void);

//管脚定义
#define LED_PIN         HI_IO_NAME_GPIO_2
#define LED_GPIO_FUN    HI_IO_FUNC_GPIO_2_GPIO

// LED控制宏定义
#define LED_ON()        hi_gpio_set_ouput_val(LED_PIN, HI_GPIO_VALUE0)
#define LED_OFF()       hi_gpio_set_ouput_val(LED_PIN, HI_GPIO_VALUE1)

// 系统时钟配置
static uint32_t g_tick_freq = 0;

// 全局变量
static uint32_t g_last_led_update = 0;
static uint32_t g_last_dht11_update = 0;
static uint32_t g_last_buzzer_update = 0;
static uint8_t led_value = 0;

static int16_t temp = 0;
static int16_t humi = 0;

// 常量定义（毫秒）
#define LED_UPDATE_INTERVAL_MS   250     // LED更新间隔10ms
#define BUZZER_UPDATE_INTERVAL_MS  1000    // 蜂鸣器报警间隔1000ms
#define DHT11_UPDATE_INTERVAL_MS 2000

// 获取当前时间（毫秒）
uint32_t get_current_time_ms(void) {
    if (g_tick_freq == 0) {
        return 0;
    }
    return (osKernelGetTickCount() * 1000) / g_tick_freq;
}

// 主任务
static void Main_Task(void) {
    uint32_t current_time_ms;
    uint32_t start_time_ms;
    
    // 获取系统tick频率
    g_tick_freq = osKernelGetTickFreq();
    printf("System Tick Frequency: %d Hz\n", g_tick_freq);
    printf("1 tick = %d ms\n", 1000 / g_tick_freq);
    
    // 初始化LED
    led_init();
    beep_init();
    while(dht11_init()) {
		printf("DHT11检测失败，请插好!\r\n");
		usleep(500*1000); //500ms
	}
    
    printf("System Init Success!\n");
    
    // 获取启动时间作为基准
    start_time_ms = get_current_time_ms();
    g_last_led_update = start_time_ms;
    g_last_dht11_update = start_time_ms;
    g_last_buzzer_update = start_time_ms;

    
    while (1) {
        current_time_ms = get_current_time_ms();
        
        // LED显示更新（100ms间隔）- 非阻塞延时方式
        if (current_time_ms - g_last_led_update >= LED_UPDATE_INTERVAL_MS) {
            g_last_led_update = current_time_ms;
            
            // LED闪烁控制
            if (led_value) {
                LED_ON();
            } else {
                LED_OFF();
            }
            led_value = !led_value;
            
        }

        if (current_time_ms - g_last_dht11_update >= LED_UPDATE_INTERVAL_MS) {
            g_last_dht11_update = current_time_ms;
            
            dht11_read_data(&temp,&humi);
            printf("温度:%d 湿度:%d\n",temp,humi);
        }
        
        if (current_time_ms - g_last_buzzer_update >= BUZZER_UPDATE_INTERVAL_MS) {
            g_last_buzzer_update = current_time_ms;
            
            beep_alarm(1,100*1000);s
        }
    }
}

static void Main_Entry(void)
{
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
        printf("Failed to create Main_Task!\n");
    }
}

APP_FEATURE_INIT(Main_Entry);

void led_init(void) {
    hi_gpio_init();                                            // GPIO初始化
    hi_io_set_pull(LED_PIN, HI_IO_PULL_DOWN);                  // 设置GPIO下拉
    hi_io_set_func(LED_PIN, LED_GPIO_FUN);                     // 设置IO为GPIO功能
    hi_gpio_set_dir(LED_PIN, HI_GPIO_DIR_OUT);                 // 设置GPIO为输出模式
}