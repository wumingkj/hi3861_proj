#include "buzzer.h"
#include "hi_io.h"
#include "hi_gpio.h"
#include <unistd.h>

// 蜂鸣器引脚定义 - 使用GPIO14（根据示例代码）
#define BEEP_PIN         HI_IO_NAME_GPIO_14
#define BEEP_GPIO_FUN    HI_IO_FUNC_GPIO_14_GPIO

// 蜂鸣器控制宏 - 修复参数类型
#define BEEP_ON()        hi_gpio_set_ouput_val(HI_GPIO_IDX_14, HI_GPIO_VALUE1)
#define BEEP_OFF()       hi_gpio_set_ouput_val(HI_GPIO_IDX_14, HI_GPIO_VALUE0)

// 蜂鸣器初始化
void Buzzer_Init(void)
{
    hi_gpio_init();                                            // GPIO初始化
    hi_io_set_pull(BEEP_PIN, HI_IO_PULL_UP);                   // 设置GPIO上拉
    hi_io_set_func(BEEP_PIN, BEEP_GPIO_FUN);                   // 设置IO为GPIO功能
    hi_gpio_set_dir(HI_GPIO_IDX_14, HI_GPIO_DIR_OUT);          // 设置GPIO为输出模式
    BEEP_OFF();                                                // 初始状态关闭蜂鸣器
    printf("Buzzer Initialized Success!\n");
}

// 蜂鸣器鸣叫
void Buzzer_Beep(uint16_t duration_ms)
{
    BEEP_ON();
    usleep(duration_ms * 1000);
    BEEP_OFF();
}

// 蜂鸣器模式鸣叫
void Buzzer_BeepPattern(uint16_t on_time, uint16_t off_time, uint8_t count)
{
    while(count--)
    {
        BEEP_ON();
        usleep(on_time * 1000);
        BEEP_OFF();
        usleep(off_time * 1000);
    }
}