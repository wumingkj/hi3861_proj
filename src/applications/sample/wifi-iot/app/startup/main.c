/*
 * Copyright (c) 2020 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ohos_init.h"
#include "ohos_types.h"
#include "cmsis_os2.h"
#include <stdio.h>
#include <stdlib.h>
// 使用Hi3861项目中的hilog_lite日志系统
#include "hiview_log.h"

#define TAG "Hi3861_Framework"

// 定义GPIO引脚
#define LED_GPIO 2  // GPIO2 连接LED
#define BUTTON_GPIO 9  // GPIO9 连接按钮

// 全局变量
static osTimerId_t g_ledTimerId = NULL;
static uint32_t g_ledState = 0;

// 简单的GPIO控制函数（替代wifiiot_gpio.h中的函数）
static void GpioSetOutputVal(uint32_t gpio, uint32_t val)
{
    // 这里使用Hi3861的日志系统输出代替实际的GPIO控制
    HILOG_INFO(HILOG_MODULE_APP, "GPIO%d 设置为: %d", gpio, val);
}

static void IoSetFunc(uint32_t gpio, uint32_t func)
{
    HILOG_INFO(HILOG_MODULE_APP, "设置GPIO%d功能: %d", gpio, func);
}

static void GpioSetDir(uint32_t gpio, uint32_t dir)
{
    HILOG_INFO(HILOG_MODULE_APP, "设置GPIO%d方向: %s", gpio, dir ? "输出" : "输入");
}

static void GpioRegisterIsrFunc(uint32_t gpio, uint32_t intType, uint32_t edge, void (*isrFunc)(char*), char* arg)
{
    HILOG_INFO(HILOG_MODULE_APP, "注册GPIO%d中断处理函数", gpio);
}

// LED闪烁任务
static void LedBlinkTask(void *argument)
{
    (void)argument;
    
    // 切换LED状态
    g_ledState = !g_ledState;
    GpioSetOutputVal(LED_GPIO, g_ledState);
    
    HILOG_INFO(HILOG_MODULE_APP, "LED状态: %s", g_ledState ? "ON" : "OFF");
}

// 按钮中断处理函数
static void ButtonIsrFunc(char *arg)
{
    (void)arg;
    HILOG_INFO(HILOG_MODULE_APP, "按钮被按下");
    
    // 可以在这里添加按钮处理逻辑
}

// 系统初始化函数
static void SystemInit(void)
{
    HILOG_INFO(HILOG_MODULE_APP, "系统初始化开始");
    
    // 初始化GPIO
    IoSetFunc(LED_GPIO, 0); // 使用0作为默认功能
    GpioSetDir(LED_GPIO, 1); // 1表示输出方向
    
    IoSetFunc(BUTTON_GPIO, 0);
    GpioSetDir(BUTTON_GPIO, 0); // 0表示输入方向
    GpioRegisterIsrFunc(BUTTON_GPIO, 0, 0, ButtonIsrFunc, NULL);
    
    HILOG_INFO(HILOG_MODULE_APP, "GPIO初始化完成");
}

// 创建定时器任务
static void CreateTimerTasks(void)
{
    // 创建LED闪烁定时器（1秒间隔）
    osTimerAttr_t ledTimerAttr = {
        .name = "LedTimer",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U
    };
    
    g_ledTimerId = osTimerNew(LedBlinkTask, osTimerPeriodic, NULL, &ledTimerAttr);
    if (g_ledTimerId != NULL) {
        osTimerStart(g_ledTimerId, 1000U); // 1秒间隔
        HILOG_INFO(HILOG_MODULE_APP, "LED定时器创建成功");
    } else {
        HILOG_ERROR(HILOG_MODULE_APP, "LED定时器创建失败");
    }
}

// 主任务函数
static void MainTask(void *argument)
{
    (void)argument;
    
    HILOG_INFO(HILOG_MODULE_APP, "主任务开始运行");
    
    // 系统初始化
    SystemInit();
    
    // 创建定时器任务
    CreateTimerTasks();
    
    // 主循环
    while (1) {
        // 可以在这里添加其他任务逻辑
        osDelay(1000); // 1秒延迟
    }
}

// 应用程序入口函数
static void AppEntry(void)
{
    HILOG_INFO(HILOG_MODULE_APP, "应用程序入口");
    
    osThreadAttr_t attr;
    
    // 设置任务属性
    attr.name = "MainTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 4096; // 4KB栈空间
    attr.priority = osPriorityNormal;
    
    // 创建主任务
    if (osThreadNew((osThreadFunc_t)MainTask, NULL, &attr) == NULL) {
        HILOG_ERROR(HILOG_MODULE_APP, "创建主任务失败");
    } else {
        HILOG_INFO(HILOG_MODULE_APP, "主任务创建成功");
    }
}

// 系统运行宏
SYS_RUN(AppEntry);