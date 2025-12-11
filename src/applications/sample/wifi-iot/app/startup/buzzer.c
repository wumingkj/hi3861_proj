#include "buzzer.h"
#include <stdio.h>

static hi_gpio_value g_buzzerState = HI_GPIO_VALUE0;

// 蜂鸣器初始化
void Buzzer_Init(void) {
    // 初始化GPIO模块
    hi_u32 ret = hi_gpio_init();
    if (ret != 0) {  // HI_ERR_SUCCESS 通常是 0
        printf("[ERROR] GPIO init failed: %u\n", ret);
        return;
    }
    
    // 设置GPIO为输出模式
    ret = hi_gpio_set_dir(BUZZER_GPIO_IDX, HI_GPIO_DIR_OUT);
    if (ret != 0) {
        printf("[ERROR] Set GPIO direction failed: %u\n", ret);
        return;
    }
    
    // 初始状态为关闭
    ret = hi_gpio_set_ouput_val(BUZZER_GPIO_IDX, HI_GPIO_VALUE0);
    if (ret != 0) {
        printf("[ERROR] Set GPIO value failed: %u\n", ret);
        return;
    }
    
    g_buzzerState = HI_GPIO_VALUE0;
    printf("[INFO] Buzzer initialized on GPIO %d\n", BUZZER_GPIO_IDX);
}

// 设置蜂鸣器状态
void Buzzer_SetState(BuzzerState state) {
    hi_u32 ret = hi_gpio_set_ouput_val(BUZZER_GPIO_IDX, state);
    if (ret != 0) {
        printf("[ERROR] Set buzzer state failed: %u\n", ret);
        return;
    }
    g_buzzerState = state;
}

// 单次蜂鸣
void Buzzer_Beep(uint32_t duration_ms) {
    printf("[INFO] Buzzer beep for %u ms\n", duration_ms);
    
    Buzzer_SetState(BUZZER_ON);
    osDelay(duration_ms);  // 注意：osDelay 单位是毫秒
    Buzzer_SetState(BUZZER_OFF);
}

// 蜂鸣模式
void Buzzer_BeepPattern(uint32_t beep_ms, uint32_t silence_ms, uint8_t times) {
    printf("[INFO] Buzzer pattern: beep=%u, silence=%u, times=%u\n", 
           beep_ms, silence_ms, times);
    
    for (uint8_t i = 0; i < times; i++) {
        Buzzer_SetState(BUZZER_ON);
        osDelay(beep_ms);
        Buzzer_SetState(BUZZER_OFF);
        
        if (i < times - 1) {
            osDelay(silence_ms);
        }
    }
}

// 蜂鸣器反初始化
void Buzzer_Deinit(void) {
    Buzzer_SetState(BUZZER_OFF);
    hi_gpio_deinit();
    printf("[INFO] Buzzer deinitialized\n");
}