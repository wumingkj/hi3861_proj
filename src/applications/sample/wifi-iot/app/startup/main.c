#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "oled.h"
#include "buzzer.h"
#include "dht11.h"

// 全局变量用于存储传感器数据
static float g_temperature = -999.9f;  // 初始化为-999.9f
static int g_humidity = -1;            // 初始化为-1
static bool g_dht11_connected = false;
static uint32_t g_counter = 0;
static osMutexId_t g_data_mutex = NULL;

// 时间戳变量用于非阻塞延时
static uint32_t g_last_sensor_update = 0;
static uint32_t g_last_oled_update = 0;
static uint32_t g_last_buzzer_check = 0;

// 新增：连续读取失败计数
static int g_failed_count = 0;
#define MAX_FAILED_COUNT 10  // 连续5次失败才显示"not found"

// 常量定义
#define SENSOR_UPDATE_INTERVAL 2000    // 增加到2秒读取一次，减少干扰
#define OLED_UPDATE_INTERVAL   500     // 500ms
#define BUZZER_CHECK_INTERVAL  500     // 500ms
#define COUNTER_UPDATE_INTERVAL 500    // 500ms
#define MAX_FAILED_COUNT 3             // 减少到连续3次失败就显示"not found"

// DHT11读取重试机制
#define DHT11_RETRY_COUNT 3            // 每次读取最多重试3次

// 主任务 - 优化版本
static void Main_Task(void)
{
    uint32_t current_time;
    
    // 初始化OLED和蜂鸣器
    OLED_Init();
    Buzzer_Init();
    
    // 初始化DHT11
    if (dht11_init() == 0) {
        printf("DHT11 Init Success!\n");
        g_dht11_connected = true;
    } else {
        printf("DHT11 Init Failed!\n");
        g_dht11_connected = false;
    }
    
    printf("System Init Success! (Optimized Single Task Mode)\n");
    
    // 启动提示音
    Buzzer_BeepPattern(100, 50, 2);
    
    // 初始化时间戳
    g_last_sensor_update = osKernelGetTickCount();
    g_last_oled_update = osKernelGetTickCount();
    g_last_buzzer_check = osKernelGetTickCount();
    uint32_t last_counter_update = osKernelGetTickCount();
    
    // OLED显示相关变量
    static float last_temperature = -999.9f;
    static int last_humidity = -1;
    static uint32_t last_counter = 0;
    static bool last_connected = false;
    
    // 初始化显示内容
    OLED_RequestShowString(0, 0, "Temp: -999.9 C", 8);
    OLED_RequestShowString(0, 10, "Humi: -1 %", 8);
    OLED_RequestShowString(0, 20, "Counter: 0", 8);
    
    // 给DHT11足够的稳定时间
    osDelay(2000); // 等待2秒让DHT11完全稳定
    
    while (1) {
        current_time = osKernelGetTickCount();
        
        // 1. 传感器读取（2秒间隔，减少干扰）
        if (current_time - g_last_sensor_update >= SENSOR_UPDATE_INTERVAL) {
            uint8_t humidity, temperature;
            uint8_t retry_count = 0;
            bool read_success = false;
            
            // 重试机制：最多重试3次
            for (retry_count = 0; retry_count < DHT11_RETRY_COUNT; retry_count++) {
                // 直接调用DHT11库函数读取数据
                if (dht11_read_data(&temperature, &humidity) == 0) {
                    read_success = true;
                    break;
                }
                
                // 如果失败，等待一小段时间再重试
                if (retry_count < DHT11_RETRY_COUNT - 1) {
                    osDelay(10); // 等待10ms再重试
                }
            }
            
            if (read_success) {
                // 使用互斥锁保护共享数据
                if (osMutexAcquire(g_data_mutex, 0) == osOK) {
                    g_humidity = (int)humidity;
                    g_temperature = (float)temperature;
                    g_dht11_connected = true;
                    g_failed_count = 0;  // 重置失败计数
                    osMutexRelease(g_data_mutex);
                }
                // 读取成功时打印信息
                printf("DHT11 Read Success (重试%d次): Humidity=%d%%, Temperature=%.1f°C\n", 
                       retry_count, humidity, (float)temperature);
            } else {
                // 读取失败
                if (osMutexAcquire(g_data_mutex, 0) == osOK) {
                    g_failed_count++;
                    if (g_failed_count >= MAX_FAILED_COUNT) {
                        g_dht11_connected = false;
                        printf("DHT11 Read Failed (连续%d次失败)\n", g_failed_count);
                    } else {
                        printf("DHT11 Read Failed (第%d次失败)\n", g_failed_count);
                    }
                    osMutexRelease(g_data_mutex);
                }
            }
            
            g_last_sensor_update = current_time;
        }
        
        // 2. 计数器更新（500ms间隔）
        if (current_time - last_counter_update >= COUNTER_UPDATE_INTERVAL) {
            if (osMutexAcquire(g_data_mutex, 0) == osOK) {
                g_counter++;
                osMutexRelease(g_data_mutex);
            }
            last_counter_update = current_time;
        }
        
        // 3. 蜂鸣器控制（500ms间隔）
        if (current_time - g_last_buzzer_check >= BUZZER_CHECK_INTERVAL) {
            if (g_counter % 80 == 0) {  // 调整为80，减少蜂鸣频率
                Buzzer_Beep(100);
            }
            
            Buzzer_Update();
            g_last_buzzer_check = current_time;
        }
        
        // 4. OLED显示更新（500ms间隔，但只有数据变化时才更新）
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
                // 清空显示缓冲区 - 修复：使用正确的函数
                oled_driver_clear_backbuffer();  // 使用正确的清除函数
                
                // 更新显示内容
                if (g_dht11_connected) {
                    char temp_str[20], humi_str[20];
                    if (g_temperature == -999.9f) {
                        snprintf(temp_str, sizeof(temp_str), "Temp: -- C");
                    } else {
                        snprintf(temp_str, sizeof(temp_str), "Temp: %.1f C", g_temperature);
                    }
                    
                    if (g_humidity == -1) {
                        snprintf(humi_str, sizeof(humi_str), "Humi: -- %%");
                    } else {
                        snprintf(humi_str, sizeof(humi_str), "Humi: %d %%", g_humidity);
                    }
                    
                    OLED_RequestShowString(0, 0, temp_str, 8);
                    OLED_RequestShowString(0, 10, humi_str, 8);
                } else {
                    OLED_RequestShowString(0, 0, "DHT11 Not Found!", 8);
                    OLED_RequestShowString(0, 10, "Check GPIO7", 8);  // 修正为GPIO7
                }
                
                char counter_str[20];
                snprintf(counter_str, sizeof(counter_str), "Counter: %lu", g_counter);
                OLED_RequestShowString(0, 20, counter_str, 8);
            }
            
            g_last_oled_update = current_time;
        }
        
        // 统一的延时，让出CPU
        osDelay(200); // 增加到200ms，大大减少CPU占用
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
    
    // 只创建一个主任务
    osThreadAttr_t main_attr = {
        .name = "MainTask",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U,
        .stack_mem = NULL,  // 设置为NULL让系统自动分配
        .stack_size = 8192, // 适当增加栈大小
        .priority = osPriorityNormal
    };
    
    // 创建单个主任务
    if (osThreadNew((osThreadFunc_t)Main_Task, NULL, &main_attr) == NULL) {
        printf("Failed to create Main_Task!\n");
    }
}

APP_FEATURE_INIT(Main_Entry);