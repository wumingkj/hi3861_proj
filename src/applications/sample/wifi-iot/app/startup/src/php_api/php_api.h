/**
 ****************************************************************************************************
 * @file        php_api.h
 * @author      Hi3861 WiFi-IoT Project
 * @version     V1.0
 * @date        2024-12-19
 * @brief       PHP API模块 - HTTP服务器和Web接口
 * @license     Copyright (c) 2024, Hi3861 WiFi-IoT Project
 ****************************************************************************************************
 * @attention
 *
 * 提供HTTP服务器功能，支持PHP客户端API接口
 * 专注API服务，网络功能由network任务处理
 *
 */

#ifndef PHP_API_H
#define PHP_API_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "cmsis_os2.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "wifi.h"
#include "time.h"
#include "../debug.h"

// HTTP服务器配置
#define PHP_API_SERVER_PORT         80
#define PHP_API_MAX_CLIENTS         5
#define PHP_API_BUFFER_SIZE         1024
#define PHP_API_REQUEST_TIMEOUT_MS  30000

// API路径定义
#define API_PATH_STATUS             "/api/status"
#define API_PATH_SENSOR_DATA        "/api/sensor/data"
#define API_PATH_SYSTEM_INFO        "/api/system/info"

// PHP API返回码
typedef enum {
    PHP_API_SUCCESS = 0,
    PHP_API_ERROR_GENERAL = -1,
    PHP_API_ERROR_INVALID_PARAM = -2,
    PHP_API_ERROR_SERVER_START_FAILED = -3
} php_api_result_t;

// 系统状态结构体
typedef struct {
    char device_name[32];
    char firmware_version[16];
    uint32_t uptime_ms;
    uint8_t temperature;
    uint8_t humidity;
    char wifi_status[16];
    char ip_address[16];
} system_status_t;

// 函数声明

/**
 * @brief 初始化PHP API模块
 * @return php_api_result_t 初始化结果
 */
php_api_result_t php_api_init(void);

/**
 * @brief 启动HTTP服务器
 * @return php_api_result_t 启动结果
 */
php_api_result_t php_api_start_server(void);

/**
 * @brief 停止HTTP服务器
 * @return php_api_result_t 停止结果
 */
php_api_result_t php_api_stop_server(void);

/**
 * @brief 获取系统状态信息
 * @param status 系统状态结构体指针
 * @return php_api_result_t 获取结果
 */
php_api_result_t php_api_get_system_status(system_status_t *status);

/**
 * @brief 处理HTTP客户端请求
 * @param client_socket 客户端socket
 */
void php_api_handle_client(int client_socket);

/**
 * @brief 发送HTTP响应
 * @param client_socket 客户端socket
 * @param status_code HTTP状态码
 * @param content_type 内容类型
 * @param body 响应体
 */
void php_api_send_response(int client_socket, int status_code, const char *content_type, const char *body);

#endif