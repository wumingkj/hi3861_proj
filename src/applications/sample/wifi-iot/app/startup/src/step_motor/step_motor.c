/**
 * @file        step_motor.c
 * @brief       步进电机驱动实现 (28BYJ-48 + ULN2003)
 *              基于普中科技 bsp_step_motor.c 适配
 */
#include "step_motor.h"
#include <stdio.h>
#include <unistd.h>
#include "hi_time.h"   // hi_udelay 微秒级忙等延时

void step_motor_init(void)
{
    hi_gpio_init();

    hi_io_set_pull(MOTOR_IN1_PIN, HI_IO_PULL_UP);
    hi_io_set_func(MOTOR_IN1_PIN, HI_IO_FUNC_GPIO_7_GPIO);
    hi_gpio_set_dir(MOTOR_IN1_PIN, HI_GPIO_DIR_OUT);

    hi_io_set_pull(MOTOR_IN2_PIN, HI_IO_PULL_UP);
    hi_io_set_func(MOTOR_IN2_PIN, HI_IO_FUNC_GPIO_6_GPIO);
    hi_gpio_set_dir(MOTOR_IN2_PIN, HI_GPIO_DIR_OUT);

    hi_io_set_pull(MOTOR_IN3_PIN, HI_IO_PULL_UP);
    hi_io_set_func(MOTOR_IN3_PIN, HI_IO_FUNC_GPIO_5_GPIO);
    hi_gpio_set_dir(MOTOR_IN3_PIN, HI_GPIO_DIR_OUT);

    hi_io_set_pull(MOTOR_IN4_PIN, HI_IO_PULL_UP);
    hi_io_set_func(MOTOR_IN4_PIN, HI_IO_FUNC_GPIO_1_GPIO);
    hi_gpio_set_dir(MOTOR_IN4_PIN, HI_GPIO_DIR_OUT);

    MOTOR_STOP();
    printf("步进电机初始化完成（IN1=GPIO7 IN2=GPIO6 IN3=GPIO5 IN4=GPIO1）\n");
}

/**
 * 步进电机驱动
 * 28BYJ-48 步进角 5.625°/64，减速比 1/64
 * 转一圈所需脉冲数 = 360 / 5.625 * 64 = 4096
 * 任意角度脉冲数 = angle * 4096 / 360
 *                = 64 * angle / 45  (8拍模式下)
 */
void step_motor_run(uint8_t step, uint8_t dir, uint8_t speed, uint16_t angle, uint8_t sta)
{
    if (sta != 1) {
        MOTOR_STOP();
        return;
    }

    /* 防止 speed 越界 */
    if (speed < 1) speed = 1;
    if (speed > 5) speed = 5;

    /* speed→延时(µs)：1→800  2→1500  3→3000  4→5000  5→8000 */
    static const uint32_t delay_tbl[] = { 800, 1500, 3000, 5000, 8000 };
    uint32_t phase_delay = delay_tbl[speed - 1];

    uint16_t cycles = (uint16_t)(64U * angle / 45U);  /* 总循环次数 */

    if (dir == 0) {
        /* 逆时针 */
        for (uint16_t j = 0; j < cycles; j++) {
            for (uint8_t i = 0; i < 8; i += (8 / step)) {
                switch (i) {
                    case 0: MOTOR_IN1(1); MOTOR_IN2(0); MOTOR_IN3(0); MOTOR_IN4(0); break;
                    case 1: MOTOR_IN1(1); MOTOR_IN2(1); MOTOR_IN3(0); MOTOR_IN4(0); break;
                    case 2: MOTOR_IN1(0); MOTOR_IN2(1); MOTOR_IN3(0); MOTOR_IN4(0); break;
                    case 3: MOTOR_IN1(0); MOTOR_IN2(1); MOTOR_IN3(1); MOTOR_IN4(0); break;
                    case 4: MOTOR_IN1(0); MOTOR_IN2(0); MOTOR_IN3(1); MOTOR_IN4(0); break;
                    case 5: MOTOR_IN1(0); MOTOR_IN2(0); MOTOR_IN3(1); MOTOR_IN4(1); break;
                    case 6: MOTOR_IN1(0); MOTOR_IN2(0); MOTOR_IN3(0); MOTOR_IN4(1); break;
                    case 7: MOTOR_IN1(1); MOTOR_IN2(0); MOTOR_IN3(0); MOTOR_IN4(1); break;
                }
                hi_udelay(phase_delay);
            }
        }
    } else {
        /* 顺时针 */
        for (uint16_t j = 0; j < cycles; j++) {
            for (uint8_t i = 0; i < 8; i += (8 / step)) {
                switch (i) {
                    case 0: MOTOR_IN1(1); MOTOR_IN2(0); MOTOR_IN3(0); MOTOR_IN4(1); break;
                    case 1: MOTOR_IN1(0); MOTOR_IN2(0); MOTOR_IN3(0); MOTOR_IN4(1); break;
                    case 2: MOTOR_IN1(0); MOTOR_IN2(0); MOTOR_IN3(1); MOTOR_IN4(1); break;
                    case 3: MOTOR_IN1(0); MOTOR_IN2(0); MOTOR_IN3(1); MOTOR_IN4(0); break;
                    case 4: MOTOR_IN1(0); MOTOR_IN2(1); MOTOR_IN3(1); MOTOR_IN4(0); break;
                    case 5: MOTOR_IN1(0); MOTOR_IN2(1); MOTOR_IN3(0); MOTOR_IN4(0); break;
                    case 6: MOTOR_IN1(1); MOTOR_IN2(1); MOTOR_IN3(0); MOTOR_IN4(0); break;
                    case 7: MOTOR_IN1(1); MOTOR_IN2(0); MOTOR_IN3(0); MOTOR_IN4(0); break;
                }
                hi_udelay(phase_delay);
            }
        }
    }

    MOTOR_STOP();
}