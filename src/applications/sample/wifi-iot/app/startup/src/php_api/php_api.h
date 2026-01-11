#ifndef PHP_API_H
#define PHP_API_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "wifi.h"

// PHP API返回码枚举
typedef enum {
    PHP_API_SUCCESS = 0,           // 成功
    PHP_API_ERROR_INIT_FAILED,     // 初始化失败
    PHP_API_ERROR_WIFI_NOT_CONNECTED, // WiFi未连接
    PHP_API_ERROR_SERVER_IP_NOT_SET,  // 服务器IP未设置
    PHP_API_ERROR_SOCKET_FAILED,   // Socket创建失败
    PHP_API_ERROR_CONNECT_FAILED,  // 连接失败
    PHP_API_ERROR_SEND_FAILED,     // 发送失败
    PHP_API_ERROR_INVALID_PARAM,   // 参数无效
    PHP_API_ERROR_MEMORY_ALLOC,    // 内存分配失败
    PHP_API_ERROR_TIMEOUT          // 超时
} php_api_result_t;

// 服务器配置
#define PHP_API_SERVER_PORT 80
#define PHP_API_SEND_TIMEOUT_MS 5000
#define PHP_API_BUFFER_SIZE 1024

// 系统状态结构体（包含所有配置信息）
typedef struct {
    char device_name[32];          // 设备名称
    char server_ip[16];            // 服务器IP地址
    char local_ip[16];             // 本地IP地址
    bool wifi_connected;           // WiFi连接状态
    char wifi_ssid[32];            // WiFi名称
    char user_id[32];              // 用户ID
    char bind_line[32];            // 生产线
} php_api_system_status_t;

// 全局运行状态变量声明
extern uint8_t g_running_status;

// 传感器数据结构体
typedef struct {
    float temperature;             // 温度
    float humidity;                // 湿度
    float smoke;                   // 烟雾
    bool relay_status;             // 继电器状态
    bool buzzer_status;            // 蜂鸣器状态
} sensor_data_t;

/**
 * @brief 获取WiFi连接状态
 * @return true 已连接，false 未连接
 */
bool php_api_is_wifi_connected(void);

/**
 * @brief 初始化PHP API模块
 * @param buffer_size JSON缓冲区大小（可选，0表示使用默认大小）
 * @return PHP_API_SUCCESS 成功，其他为错误码
 */
php_api_result_t php_api_init(uint32_t buffer_size);

/**
 * @brief 释放PHP API模块资源
 */
void php_api_deinit(void);

/**
 * @brief 获取系统状态信息（从KV存储读取所有配置）
 * @param status 系统状态结构体指针
 * @return PHP_API_SUCCESS 成功，其他为错误码
 */
php_api_result_t php_api_get_system_status(php_api_system_status_t *status);

/**
 * @brief 保存系统状态信息到KV存储
 * @param status 系统状态结构体指针
 * @return PHP_API_SUCCESS 成功，其他为错误码
 */
php_api_result_t php_api_save_system_status(const php_api_system_status_t *status);

/**
 * @brief 发送握手信息到服务器
 * @return PHP_API_SUCCESS 成功，其他为错误码
 */
php_api_result_t php_api_send_handshake(void);

/**
 * @brief 发送传感器数据到服务器
 * @param sensor_data 传感器数据结构体指针
 * @return PHP_API_SUCCESS 成功，其他为错误码
 */
php_api_result_t php_api_send_sensor_data(const sensor_data_t *sensor_data);

/**
 * @brief 发送JSON数据到服务器
 * @param json_data JSON格式的字符串数据
 * @return PHP_API_SUCCESS 成功，其他为错误码
 */
php_api_result_t php_api_send_json_data(const char *json_data);

/**
 * @brief 启动HTTP网页服务器
 * @return PHP_API_SUCCESS 成功，其他为错误码
 */
php_api_result_t php_api_start_web_server(void);

/**
 * @brief 处理HTTP请求
 * @param client_socket 客户端socket
 * @return PHP_API_SUCCESS 成功，其他为错误码
 */
php_api_result_t php_api_handle_http_request(int client_socket);

/**
 * @brief 停止HTTP网页服务器
 */
void php_api_stop_web_server(void);

/**
 * @brief 更新HTTP网页服务器（处理客户端连接）
 * @return PHP_API_SUCCESS 成功，其他为错误码
 */
php_api_result_t php_api_update_web_server(void);

/**
 * @brief 设置日志输出间隔
 * @param interval 日志输出间隔（每多少次发送记录一次日志）
 */
void php_api_set_log_interval(uint32_t interval);

/**
 * @brief 获取发送统计信息
 * @param success_count 成功发送次数指针
 * @param failure_count 失败发送次数指针
 * @param total_count 总发送次数指针
 */
void php_api_get_send_statistics(uint32_t *success_count, uint32_t *failure_count, uint32_t *total_count);

/**
 * @brief 重置发送统计信息
 */
void php_api_reset_statistics(void);

/**
 * @brief URL解码函数（改为静态声明）
 * @param src 源字符串
 * @param dst 目标缓冲区
 * @param dst_size 目标缓冲区大小
 */
static void php_api_url_decode(const char *src, char *dst, size_t dst_size);

#endif // PHP_API_H