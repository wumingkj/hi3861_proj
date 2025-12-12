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

// 传感器数据更新任务
static void Sensor_UpdateTask(void) {
    uint8_t humidity, temperature;
    while (1) {
        // 读取DHT11温湿度数据
        bool connected = DHT11_ReadData(&humidity, &temperature);
        // 使用互斥锁保护共享数据
        if (osMutexAcquire(g_data_mutex, osWaitForever) == osOK) {
            g_humidity = humidity;
            g_temperature = temperature;
            g_dht11_connected = connected;
            osMutexRelease(g_data_mutex);
        }
        // 每2秒更新一次传感器数据
        osDelay(2000);
    }
}

// OLED显示任务
static void OLED_DisplayTask(void) {
    // 初始化OLED
    OLED_Init();
    printf("OLED Init Success!\n");
    
    while (1) {
        uint8_t humidity, temperature;
        bool connected;
        uint32_t counter;
        // 使用互斥锁保护共享数据
        if (osMutexAcquire(g_data_mutex, osWaitForever) == osOK) {
            humidity = g_humidity;
            temperature = g_temperature;
            connected = g_dht11_connected;
            counter = g_counter;
            osMutexRelease(g_data_mutex);
        }
        // OLED显示
        OLED_Clear();
        if (connected) {
            // 显示温湿度数据
            OLED_ShowString(0, 0, "Temp:", 8);
            OLED_ShowNum(40, 0, temperature, 2, 8);
            OLED_ShowChar(56, 0, 'C', 8);
            
            OLED_ShowString(0, 10, "Humi:", 8);
            OLED_ShowNum(40, 10, humidity, 2, 8);
            OLED_ShowString(56, 10, "%", 8);
            
            OLED_ShowString(0, 20, "Counter:", 8);
            OLED_ShowNum(64, 20, counter, 3, 8);
        } else {
            OLED_ShowString(0, 0, "DHT11 Not Found!", 8);
            OLED_ShowString(0, 10, "Check Connection", 8);
            OLED_ShowString(0, 20, "Counter:", 8);
            OLED_ShowNum(64, 20, counter, 3, 8);
        }
        
        OLED_Refresh();
        
        // 每250ms更新一次显示
        osDelay(250);
    }
}

// 主任务 - 计数器更新和蜂鸣器控制
static void Main_Task(void)
{
    // 初始化蜂鸣器和DHT11传感器
    Buzzer_Init();
    DHT11_Init();
    printf("Init Success!\n");
    
    // 启动提示音
    Buzzer_BeepPattern(100, 50, 2);
    
    while (1) {
        // 更新计数器
        if (osMutexAcquire(g_data_mutex, osWaitForever) == osOK) {
            g_counter++;
            osMutexRelease(g_data_mutex);
        }
        
        // 每5秒触发一次蜂鸣器
        if (g_counter % 20 == 0) {
            Buzzer_Beep(100);
        }
        
        // 更新蜂鸣器状态
        Buzzer_Update();
        
        osDelay(250);
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