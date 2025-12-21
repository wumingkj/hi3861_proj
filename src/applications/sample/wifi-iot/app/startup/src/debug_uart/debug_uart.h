/**
 ****************************************************************************************************
 * @file        debug_uart.h
 * @author      普中科技
 * @version     V1.0
 * @date        2024-06-05
 * @brief       通用串口调试模块
 * @license     Copyright (c) 2024-2034, 深圳市普中科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:普中-Hi3861
 * 在线视频:https://space.bilibili.com/2146492485
 * 公司网址:www.prechin.cn
 * 购买地址:
 *
 */

#ifndef DEBUG_UART_H
#define DEBUG_UART_H

#include "cmsis_os2.h"
#include "hi_uart.h"
#include <string.h>
#include <stdlib.h>

// 串口配置（使用USB调试串口）
#define DEBUG_UART_PORT HI_UART_IDX_0    // UART0（USB调试串口）
#define DEBUG_UART_BAUDRATE 115200       // 波特率
#define DEBUG_UART_BUFFER_SIZE 256       // 接收缓冲区大小

// 函数指针类型定义
typedef void (*uart_command_func_t)(int argc, char *argv[]);

// 命令结构体
typedef struct {
    const char *func_name;           // 函数名
    uart_command_func_t func_ptr;    // 函数指针
    int min_args;                    // 最小参数个数
    int max_args;                    // 最大参数个数
    const char *description;         // 函数描述
} uart_command_t;

// 核心函数声明
void debug_uart_init(void);
void debug_uart_register_command(const char *func_name, uart_command_func_t func_ptr, 
                                int min_args, int max_args, const char *description);
void debug_uart_process_command(const char *command);
void debug_uart_send_string(const char *str);
void debug_uart_task_handler(void);  // 新增：任务处理函数

// 内置命令函数声明（简化，只保留必要的）
void uart_cmd_help(void);
void uart_cmd_buzzer_alarm(int argc, char *argv[]);
void uart_cmd_dht11_read(void);
void uart_cmd_wifi_status(void);

#endif