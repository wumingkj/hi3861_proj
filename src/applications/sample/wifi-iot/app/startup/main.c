#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "oled.h"
#include "buzzer.h"  // 添加蜂鸣器头文件

static void OLED_Buzzer_TestTask(void)
{
    // 初始化OLED和蜂鸣器
    OLED_Init();
    Buzzer_Init();
    printf("OLED & Buzzer Init Success!\n");
    
    // 启动提示音
    Buzzer_BeepPattern(100, 50, 2); // 短促两声提示
    
    uint32_t counter = 0;
    
    while (1) {
        // OLED显示
        OLED_Clear();
        OLED_ShowString(0, 0, "HI3861 OLED", 8);
        OLED_ShowString(0, 16, "BUZZER DEMO", 8);
        OLED_ShowString(0, 32, "TEST123456", 8);
        OLED_ShowNum(0, 48, counter, 5, 8);
        OLED_Refresh();
        
        // 蜂鸣器控制逻辑
        if (counter % 5 == 0) {
            // 每5秒长响一次
            Buzzer_Beep(500);
        } else if (counter % 2 == 0) {
            // 每2秒短响一次
            Buzzer_Beep(100);
        }
        
        counter++;
        osDelay(1000); // 延时1秒
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
    attr.stack_size = 4096;
    attr.priority = osPriorityNormal;
    
    if (osThreadNew((osThreadFunc_t)OLED_Buzzer_TestTask, NULL, &attr) == NULL) {
        printf("Failed to create OLED_Buzzer_TestTask!\n");
    }
}

APP_FEATURE_INIT(OLED_ExampleEntry);