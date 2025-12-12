#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "oled.h"
#include "buzzer.h"
#include "dht11.h"

static void MainTask(void) {
    // 初始化OLED、蜂鸣器和DHT11传感器
    OLED_Init();
    Buzzer_Init();
    DHT11_Init();
    printf("Init Success!\n");

    const char *str = "Hello, World!";
    int len = strlen(str);  // 任务循环 - 显示字符串
    for (int i = 0; i < len; i++) {
        OLED_ShowChar(i * 8, 0, str[i], 8);
        OLED_Refresh();
        osDelay(100);
    }
    
    // 启动提示音（非阻塞方式）
    Buzzer_BeepPattern(100, 50, 2); // 短促两声提示
    
    uint32_t counter = 0;
    uint8_t humidity = 0;
    uint8_t temperature = 0;
    bool dht11_connected = false;
    
    while (1) {
        // 读取DHT11温湿度数据
        dht11_connected = DHT11_ReadData(&humidity, &temperature);
        
        // OLED显示
        OLED_Clear();
        if (dht11_connected) {
            // 显示温湿度数据
            OLED_ShowString(0, 0, "Temp:", 8);
            OLED_ShowNum(40, 0, temperature, 2, 8);
            OLED_ShowChar(56, 0, 'C', 8);
            
            OLED_ShowString(0, 10, "Humi:", 8);
            OLED_ShowNum(40, 10, humidity, 2, 8);
            OLED_ShowString(56, 10, "%", 8);
            
        } else {
            OLED_ShowString(0, 0, "DHT11 Not Found!", 8);
            OLED_ShowString(0, 10, "Check Connection", 8);
            OLED_ShowString(0, 20, "Counter:", 8);
            OLED_ShowNum(64, 40, counter, 3, 8);
        }
        
        OLED_Refresh();
        
        // 每5秒更新一次蜂鸣器状态
        if (counter % 20 == 0) {
            Buzzer_Beep(100); // 短促提示音
        }
        
        counter++;
        osDelay(250); // 延时250毫秒
    }
}

static void Main_Entry(void)
{
    osThreadAttr_t attr;
    
    attr.name = "MainTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = 4096;
    attr.stack_size = 4096;
    attr.priority = osPriorityNormal;
    
    if (osThreadNew((osThreadFunc_t)MainTask, NULL, &attr) == NULL) {
        printf("Failed to create MainTask!\n");
    }
}

APP_FEATURE_INIT(Main_Entry);