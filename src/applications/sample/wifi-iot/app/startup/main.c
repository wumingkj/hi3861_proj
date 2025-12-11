#include <stdio.h>
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
    printf("OLED & Buzzer Init Success!\n");

    OLED_ShowString(0, 0, "hello world!", 8);
    
    // 启动提示音
    Buzzer_BeepPattern(100, 50, 2); // 短促两声提示
    
    uint32_t counter = 0;
    
    while (1) {
        // OLED显示
        OLED_Clear();
        OLED_ShowString(0, 0, "HI3861 OLED", 8);
        OLED_ShowString(0, 8, "BUZZER DEMO", 8);
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
        osDelay(250); // 延时1秒
    }
}

static void Main_Entry(void)
{
    osThreadAttr_t attr;
    
    attr.name = "Main_Task";
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

APP_FEATURE_INIT(Main_Entry);