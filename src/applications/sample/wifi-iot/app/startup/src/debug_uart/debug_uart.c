/**
 ****************************************************************************************************
 * @file        debug_uart.c
 * @author      普中科技
 * @version     V1.0
 * @date        2024-06-05
 * @brief       通用串口调试模块实现
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

#include "debug_uart.h"
#include "buzzer.h"
#include "dht11.h"
#include "wifi.h"

#include "../debug.h"

// 命令表
static uart_command_t g_uart_commands[32] = {0};
static int g_command_count = 0;

// 接收缓冲区
static char g_uart_buffer[DEBUG_UART_BUFFER_SIZE] = {0};
static int g_buffer_index = 0;

// 串口接收处理标志
static volatile bool g_uart_data_ready = false;

// 内置命令函数（保留核心功能）
void uart_cmd_help(void) {
    debug_uart_send_string("=== 串口调试命令列表 ===\r\n");
    for (int i = 0; i < g_command_count; i++) {
        char help_str[128];
        snprintf(help_str, sizeof(help_str), "%s (参数: %d-%d) - %s\r\n", 
                g_uart_commands[i].func_name,
                g_uart_commands[i].min_args,
                g_uart_commands[i].max_args,
                g_uart_commands[i].description);
        debug_uart_send_string(help_str);
    }
}

// 蜂鸣器控制命令
void uart_cmd_buzzer_alarm(int argc, char *argv[]) {
    if (argc != 3) {
        debug_uart_send_string("用法: Buzzer_Alarm(count, freq, duration)\r\n");
        return;
    }
    
    int count = atoi(argv[0]);
    int freq = atoi(argv[1]);
    int duration = atoi(argv[2]);
    
    Buzzer_Alarm(count, freq, duration);
    
    char result[64];
    snprintf(result, sizeof(result), "蜂鸣器警报: count=%d, freq=%d, duration=%d\r\n", 
             count, freq, duration);
    debug_uart_send_string(result);
}

// DHT11传感器读取命令
void uart_cmd_dht11_read(void) {
    uint8_t temperature, humidity;
    
    if (dht11_read_data(&temperature, &humidity) == 0) {
        char result[64];
        snprintf(result, sizeof(result), "DHT11读取: 温度=%d°C, 湿度=%d%%\r\n", 
                 temperature, humidity);
        debug_uart_send_string(result);
    } else {
        debug_uart_send_string("DHT11读取失败\r\n");
    }
}

// WiFi状态查询命令
void uart_cmd_wifi_status(void) {
    char *ip_addr = WiFi_GetLocalIP();
    char result[128];
    
    snprintf(result, sizeof(result), "WiFi状态: IP=%s\r\n", ip_addr ? ip_addr : "未连接");
    debug_uart_send_string(result);
}

// 字符串分割函数
static int split_string(char *str, char *argv[], int max_args, const char *delimiters) {
    int argc = 0;
    char *token = strtok(str, delimiters);
    
    while (token != NULL && argc < max_args) {
        argv[argc++] = token;
        token = strtok(NULL, delimiters);
    }
    
    return argc;
}

// 解析函数调用格式的命令
static int parse_function_call(const char *command, char *func_name, char *argv[], int max_args) {
    char buffer[DEBUG_UART_BUFFER_SIZE];
    strncpy(buffer, command, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    // 查找函数名结束位置（第一个括号）
    char *paren_pos = strchr(buffer, '(');
    if (paren_pos == NULL) {
        return -1; // 没有找到括号
    }
    
    // 提取函数名
    *paren_pos = '\0';
    strncpy(func_name, buffer, DEBUG_UART_BUFFER_SIZE - 1);
    
    // 查找参数开始位置（括号后）
    char *args_start = paren_pos + 1;
    
    // 查找参数结束位置（最后一个括号）
    char *end_paren = strrchr(args_start, ')');
    if (end_paren == NULL) {
        return -1; // 没有找到结束括号
    }
    *end_paren = '\0';
    
    // 分割参数
    return split_string(args_start, argv, max_args, ", ");
}

// 查找命令
static uart_command_t* find_command(const char *func_name) {
    for (int i = 0; i < g_command_count; i++) {
        if (strcmp(g_uart_commands[i].func_name, func_name) == 0) {
            return &g_uart_commands[i];
        }
    }
    return NULL;
}

// 串口接收回调函数（简化版）
static void uart_receive_callback(void *arg) {
    (void)arg;
    char ch;
    
    while (hi_uart_read(DEBUG_UART_PORT, (uint8_t*)&ch, 1) > 0) {
        // 处理回车或换行符
        if (ch == '\r' || ch == '\n') {
            if (g_buffer_index > 0) {
                g_uart_buffer[g_buffer_index] = '\0';
                g_uart_data_ready = true;  // 设置数据就绪标志
                g_buffer_index = 0;
            }
        } 
        // 处理退格键
        else if (ch == '\b' || ch == 0x7F) {
            if (g_buffer_index > 0) {
                g_buffer_index--;
                debug_uart_send_string("\b \b"); // 回显退格
            }
        }
        // 普通字符
        else if (g_buffer_index < DEBUG_UART_BUFFER_SIZE - 1) {
            g_uart_buffer[g_buffer_index++] = ch;
            hi_uart_write(DEBUG_UART_PORT, (uint8_t*)&ch, 1); // 回显
        }
    }
}

// 初始化串口调试模块（简化版）
void debug_uart_init(void) {
    // 初始化UART（使用UART0，即USB调试串口）
    hi_uart_attribute uart_attr = {
        .baud_rate = DEBUG_UART_BAUDRATE,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 0
    };
    
    hi_uart_extra_attr extra_attr = {
        .tx_fifo_line = HI_FIFO_LINE_HALF,
        .rx_fifo_line = HI_FIFO_LINE_ONE_QUARTER,
        .flow_fifo_line = HI_FIFO_LINE_THREE_QUARTERS,
        .tx_block = HI_UART_BLOCK_STATE_BLOCK,
        .rx_block = HI_UART_BLOCK_STATE_BLOCK,
        .tx_buf_size = 0,
        .rx_buf_size = 0,
        .tx_use_dma = HI_UART_NONE_DMA,
        .rx_use_dma = HI_UART_NONE_DMA
    };
    
    hi_uart_init(DEBUG_UART_PORT, &uart_attr, &extra_attr);
    
    // 注册核心命令
    debug_uart_register_command("help", uart_cmd_help, 0, 0, "显示帮助信息");
    debug_uart_register_command("Buzzer_Alarm", uart_cmd_buzzer_alarm, 3, 3, "蜂鸣器警报");
    debug_uart_register_command("DHT11_Read", uart_cmd_dht11_read, 0, 0, "读取温湿度");
    debug_uart_register_command("WiFi_Status", uart_cmd_wifi_status, 0, 0, "查询WiFi状态");
    
    debug_uart_send_string("\r\n=== Hi3861 串口调试模块已启动 ===\r\n");
    debug_uart_send_string("输入 'help' 查看可用命令\r\n");
    debug_uart_send_string("支持函数调用格式: Buzzer_Alarm(2, 50, 100)\r\n> ");
}

// 注册命令
void debug_uart_register_command(const char *func_name, uart_command_func_t func_ptr, 
                                int min_args, int max_args, const char *description) {
    if (g_command_count >= 32) {
        log_e("UART", "命令表已满，无法注册新命令");
        return;
    }
    
    g_uart_commands[g_command_count].func_name = func_name;
    g_uart_commands[g_command_count].func_ptr = func_ptr;
    g_uart_commands[g_command_count].min_args = min_args;
    g_uart_commands[g_command_count].max_args = max_args;
    g_uart_commands[g_command_count].description = description;
    g_command_count++;
    
    log_i("UART", "注册命令: %s", func_name);
}

// 处理命令
void debug_uart_process_command(const char *command) {
    char func_name[64];
    char *argv[16];
    int argc;
    
    // 解析函数调用格式
    argc = parse_function_call(command, func_name, argv, 16);
    
    if (argc < 0) {
        // 如果不是函数调用格式，尝试作为简单命令处理
        char buffer[DEBUG_UART_BUFFER_SIZE];
        strncpy(buffer, command, sizeof(buffer) - 1);
        argc = split_string(buffer, argv, 16, " ");
        if (argc > 0) {
            strncpy(func_name, argv[0], sizeof(func_name) - 1);
        } else {
            debug_uart_send_string("错误: 无效的命令格式\r\n");
            return;
        }
    }
    
    // 查找命令
    uart_command_t *cmd = find_command(func_name);
    if (cmd == NULL) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "错误: 未知命令 '%s'\r\n", func_name);
        debug_uart_send_string(error_msg);
        return;
    }
    
    // 检查参数个数
    if (argc - 1 < cmd->min_args || argc - 1 > cmd->max_args) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), 
                "错误: 参数个数不正确，需要 %d-%d 个参数，实际 %d 个\r\n", 
                cmd->min_args, cmd->max_args, argc - 1);
        debug_uart_send_string(error_msg);
        return;
    }
    
    // 执行命令（跳过函数名参数）
    cmd->func_ptr(argc - 1, argc > 1 ? &argv[1] : NULL);
    
    debug_uart_send_string("> ");
}

// 发送字符串到串口
void debug_uart_send_string(const char *str) {
    if (str != NULL) {
        hi_uart_write(DEBUG_UART_PORT, (uint8_t*)str, strlen(str));
    }
}

// 串口调试任务处理函数（50ms循环调用）
void debug_uart_task_handler(void) {
    static uint32_t last_process_time = 0;
    uint32_t current_time = osKernelGetTickCount();
    
    // 每50ms处理一次
    if (current_time - last_process_time >= 50) {
        last_process_time = current_time;
        
        // 检查是否有串口数据需要处理
        if (g_uart_data_ready) {
            g_uart_data_ready = false;
            debug_uart_process_command(g_uart_buffer);
        }
        
        // 处理串口接收（非阻塞方式）
        uart_receive_callback(NULL);
    }
}