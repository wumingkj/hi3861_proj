#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "oled.h"
#include "buzzer.h"
#include "dht11.h"

// 全局变量用于存储传感器数据
static uint8_t g_humidity = 0;
static uint8_t g_temperature = 0;
static bool g_dht11_connected = false;
static uint32_t g_counter = 0;
static osMutexId_t g_data_mutex = NULL;

// 时间戳变量用于非阻塞延时
static uint32_t g_last_sensor_update = 0;
static uint32_t g_last_oled_update = 0;
static uint32_t g_last_buzzer_check = 0;

// 常量定义
#define SENSOR_UPDATE_INTERVAL 2000  // 2秒
#define OLED_UPDATE_INTERVAL   10   // 25ms
#define BUZZER_CHECK_INTERVAL  250   // 250ms

// 传感器数据更新任务（非阻塞版本）
static void Sensor_UpdateTask(void) {
    uint8_t humidity, temperature;
    uint32_t current_time;
    
    while (1) {
        current_time = osKernelGetTickCount();
        
        // 非阻塞检查：是否到了传感器更新时间
        if (current_time - g_last_sensor_update >= SENSOR_UPDATE_INTERVAL) {
            // 读取DHT11温湿度数据
            bool connected = DHT11_ReadData(&humidity, &temperature);
            // 使用互斥锁保护共享数据
            if (osMutexAcquire(g_data_mutex, 0) == osOK) {  // 非阻塞获取锁
                g_humidity = humidity;
                g_temperature = temperature;
                g_dht11_connected = connected;
                osMutexRelease(g_data_mutex);
            }
            g_last_sensor_update = current_time;
        }
        
        // 短延时，让出CPU给其他任务
        osDelay(10); // 只阻塞10ms
    }
}

// OLED显示任务（修复版本）
void OLED_DisplayTask(void* arg)
{
    (void)arg;
    uint32_t current_time;
    static uint8_t last_temperature = 0;
    static uint8_t last_humidity = 0;
    static uint32_t last_counter = 0;
    static bool last_connected = false;
    
    // 初始化显示内容
    OLED_RequestShowString(0, 0, "Temp: -- C", 8);
    OLED_RequestShowString(0, 10, "Humi: -- %", 8);
    OLED_RequestShowString(0, 20, "Counter: 0", 8);
    
    while (1) {
        current_time = osKernelGetTickCount();
        
        // 非阻塞检查：是否到了OLED更新时间
        if (current_time - g_last_oled_update >= OLED_UPDATE_INTERVAL) {
            bool need_update = false;
            
            // 检查数据是否有变化，只有变化时才更新显示
            if (osMutexAcquire(g_data_mutex, 0) == osOK) {
                if (g_temperature != last_temperature || 
                    g_humidity != last_humidity || 
                    g_counter != last_counter ||
                    g_dht11_connected != last_connected) {
                    
                    need_update = true;
                    last_temperature = g_temperature;
                    last_humidity = g_humidity;
                    last_counter = g_counter;
                    last_connected = g_dht11_connected;
                }
                osMutexRelease(g_data_mutex);
            }
            
            if (need_update) {
                // 清空后缓冲区（只清需要更新的区域）
                oled_driver_clear_backbuffer();
                
                // 更新显示内容
                if (g_dht11_connected) {
                    char temp_str[20], humi_str[20];
                    snprintf(temp_str, sizeof(temp_str), "Temp: %d C", g_temperature);
                    snprintf(humi_str, sizeof(humi_str), "Humi: %d %%", g_humidity);
                    
                    OLED_RequestShowString(0, 0, temp_str, 8);
                    OLED_RequestShowString(0, 10, humi_str, 8);
                } else {
                    OLED_RequestShowString(0, 0, "DHT11 Not Found!", 8);
                    OLED_RequestShowString(0, 10, "Check GPIO5", 8);
                }
                
                char counter_str[20];
                snprintf(counter_str, sizeof(counter_str), "Counter: %lu", g_counter);
                OLED_RequestShowString(0, 20, counter_str, 8);
            }
            
            g_last_oled_update = current_time;
        }
        
        // 短延时，让出CPU给其他任务
        osDelay(10); // 只阻塞10ms
    }
}

// 主任务 - 计数器更新和蜂鸣器控制（修复版本）
static void Main_Task(void)
{
    uint32_t current_time;
    
    // 初始化OLED、蜂鸣器和DHT11传感器
    OLED_Init();  // 这会自动启动后台刷新任务
    Buzzer_Init();
    DHT11_Init();
    printf("Init Success!\n");
    
    // 启动提示音
    Buzzer_BeepPattern(100, 50, 2);
    
    // 初始化时间戳
    g_last_sensor_update = osKernelGetTickCount();
    g_last_oled_update = osKernelGetTickCount();
    g_last_buzzer_check = osKernelGetTickCount();
    uint32_t last_counter_update = osKernelGetTickCount();
    
    while (1) {
        current_time = osKernelGetTickCount();
        
        // 非阻塞更新计数器 - 每25ms更新一次，与OLED显示同步
        if (current_time - last_counter_update >= OLED_UPDATE_INTERVAL) {
            if (osMutexAcquire(g_data_mutex, 0) == osOK) {  // 非阻塞获取锁
                g_counter++;
                osMutexRelease(g_data_mutex);
            }
            last_counter_update = current_time;
        }
        
        // 非阻塞检查：是否到了蜂鸣器检查时间
        if (current_time - g_last_buzzer_check >= BUZZER_CHECK_INTERVAL) {
            // 每5秒触发一次蜂鸣器
            if (g_counter % 20 == 0) {
                Buzzer_Beep(100);
            }
            
            // 更新蜂鸣器状态
            Buzzer_Update();
            
            g_last_buzzer_check = current_time;
        }
        
        // 短延时，让出CPU给其他任务
        osDelay(10); // 只阻塞10ms
    }
}

static void Main_Entry(void)
{
    // 创建互斥锁用于保护共享数据
    osMutexAttr_t mutex_attr = {
        .name = "DataMutex",
        .attr_bits = osMutexRecursive,
        .cb_mem = NULL,
        .cb_size = 0U
    };
    g_data_mutex = osMutexNew(&mutex_attr);
    
    if (g_data_mutex == NULL) {
        printf("Failed to create mutex!\n");
        return;
    }
    
    // 创建主任务
    osThreadAttr_t main_attr = {
        .name = "MainTask",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U,
        .stack_mem = 2048,
        .stack_size = 2048,
        .priority = osPriorityNormal
    };
    
    // 创建OLED显示任务
    osThreadAttr_t oled_attr = {
        .name = "OLEDTask",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U,
        .stack_mem = 2048,
        .stack_size = 2048,
        .priority = osPriorityNormal
    };
    
    // 创建传感器更新任务
    osThreadAttr_t sensor_attr = {
        .name = "SensorTask",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U,
        .stack_mem = 2048,
        .stack_size = 2048,
        .priority = osPriorityNormal
    };
    
    // 创建所有任务
    if (osThreadNew((osThreadFunc_t)Main_Task, NULL, &main_attr) == NULL) {
        printf("Failed to create Main_Task!\n");
    }
    
    if (osThreadNew((osThreadFunc_t)OLED_DisplayTask, NULL, &oled_attr) == NULL) {
        printf("Failed to create OLED_DisplayTask!\n");
    }
    
    if (osThreadNew((osThreadFunc_t)Sensor_UpdateTask, NULL, &sensor_attr) == NULL) {
        printf("Failed to create Sensor_UpdateTask!\n");
    }
}

APP_FEATURE_INIT(Main_Entry);