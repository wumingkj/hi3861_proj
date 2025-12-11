#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "oled.h"
#include "buzzer.h"

static void OLED_Buzzer_TestTask(void)
{
    // 初始化OLED和蜂鸣器
    OLED_Init();
    Buzzer_Init();
    printf("OLED & Buzzer PWM Init Success!\n");

    const char *str = "Hello, World!";
    int len = strlen(str);  // 任务循环 - 显示字符串
    for (int i = 0; i < len; i++) {
        OLED_ShowChar(0, i * 8, str[i], 8);
        osDelay(25);
    }
    
    // 启动提示音（非阻塞方式）
    Buzzer_BeepPattern(100, 50, 2); // 短促两声提示
    
    uint32_t counter = 0;
    
    while (1) {
        // 更新蜂鸣器状态（非阻塞方式）
        Buzzer_Update();
        
        // OLED显示
        OLED_Clear();
        OLED_ShowString(0, 0, "HI3861 OLED", 8);
        OLED_ShowString(0, 10, "BUZZER PWM", 8);
        OLED_ShowNum(0, 20, counter, 3, 8);
        OLED_Refresh();
        
        // 蜂鸣器控制逻辑（非阻塞方式）
        if (counter % 5 == 0 && !Buzzer_IsActive()) {
            // 每5秒长响一次（如果当前没有蜂鸣）
            Buzzer_Beep(500);
        } else if (counter % 2 == 0 && !Buzzer_IsActive()) {
            // 每2秒短响一次（如果当前没有蜂鸣）
            Buzzer_Beep(100);
        }
        
        counter++;
        osDelay(250); // 延时1秒
    }
}

static void OLED_ExampleEntry(void)
{
    osThreadAttr_t attr;
    
    attr.name = "OLED_Buzzer_Task";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 8192;
    attr.priority = osPriorityNormal;
    
    if (osThreadNew((osThreadFunc_t)OLED_Buzzer_TestTask, NULL, &attr) == NULL) {
        printf("Failed to create OLED_Buzzer_TestTask!\n");
    }
}

APP_FEATURE_INIT(OLED_ExampleEntry);