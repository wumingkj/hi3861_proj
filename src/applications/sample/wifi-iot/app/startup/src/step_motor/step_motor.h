/**
 * @file        step_motor.h
 * @brief       步进电机驱动 (28BYJ-48 + ULN2003)
 *              四相八拍驱动，IN1→GPIO12, IN2→GPIO13, IN3→GPIO2, IN4→GPIO8
 */
#ifndef STEP_MOTOR_H
#define STEP_MOTOR_H

#include <stdint.h>
#include "hi_io.h"
#include "hi_gpio.h"

/* 速度常量：值越小越快（最小 1） */
#define STEPMOTOR_MAXSPEED    1
#define STEPMOTOR_MINSPEED    5

/* 引脚定义 */
#define MOTOR_IN1_PIN   HI_IO_NAME_GPIO_7
#define MOTOR_IN2_PIN   HI_IO_NAME_GPIO_6
#define MOTOR_IN3_PIN   HI_IO_NAME_GPIO_5
#define MOTOR_IN4_PIN   HI_IO_NAME_GPIO_1

/* GPIO 输出宏 */
#define MOTOR_IN1(v)    hi_gpio_set_ouput_val(MOTOR_IN1_PIN, (v) ? HI_GPIO_VALUE1 : HI_GPIO_VALUE0)
#define MOTOR_IN2(v)    hi_gpio_set_ouput_val(MOTOR_IN2_PIN, (v) ? HI_GPIO_VALUE1 : HI_GPIO_VALUE0)
#define MOTOR_IN3(v)    hi_gpio_set_ouput_val(MOTOR_IN3_PIN, (v) ? HI_GPIO_VALUE1 : HI_GPIO_VALUE0)
#define MOTOR_IN4(v)    hi_gpio_set_ouput_val(MOTOR_IN4_PIN, (v) ? HI_GPIO_VALUE1 : HI_GPIO_VALUE0)

/* 所有线圈断电 */
#define MOTOR_STOP()    do { MOTOR_IN1(0); MOTOR_IN2(0); MOTOR_IN3(0); MOTOR_IN4(0); } while (0)

void step_motor_init(void);

/**
 * @brief 步进电机转动
 * @param step  步进模式：4 或 8
 * @param dir   方向：1=顺时针, 0=逆时针
 * @param speed 速度：1~5（越小越快）
 * @param angle 角度：0~360
 * @param sta   启停：1=启动, 0=停止
 */
void step_motor_run(uint8_t step, uint8_t dir, uint8_t speed, uint16_t angle, uint8_t sta);

/* 简易封装：顺时针转动指定角度 */
static inline void step_motor_cw(uint16_t angle, uint8_t speed) {
    step_motor_run(8, 1, speed, angle, 1);
}

/* 简易封装：逆时针转动指定角度 */
static inline void step_motor_ccw(uint16_t angle, uint8_t speed) {
    step_motor_run(8, 0, speed, angle, 1);
}

#endif /* STEP_MOTOR_H */