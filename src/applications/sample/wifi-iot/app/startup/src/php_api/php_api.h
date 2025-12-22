/**
 ****************************************************************************************************
 * @file        php_api.h
 * @author      Hi3861 WiFi-IoT Project
 * @version     V2.0
 * @date        2024-12-19
 * @brief       PHP API模块 - HTTP服务器和纯网络API接口
 * @license     Copyright (c) 2024, Hi3861 WiFi-IoT Project
 ****************************************************************************************************
 * @attention
 *
 * 提供HTTP服务器功能，支持纯网络交互API接口
 * 移除网页内嵌，专注设备控制API
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
#include "buzzer.h"
#include "dht11.h"
#include "hi_gpio.h"
#include "hi_io.h"
#include "kv.h"

#include "../debug.h"

// HTTP服务器配置
#define PHP_API_SERVER_PORT         80
#define PHP_API_MAX_CLIENTS         5
#define PHP_API_BUFFER_SIZE         1024
#define PHP_API_REQUEST_TIMEOUT_MS  30000

// API路径定义 - 纯网络交互API
#define API_PATH_STATUS             "/api/status"
#define API_PATH_SENSOR_DATA        "/api/sensor/data"
#define API_PATH_SENSOR_TEMPERATURE "/api/sensor/temperature"
#define API_PATH_SENSOR_HUMIDITY    "/api/sensor/humidity"
#define API_PATH_BUZZER_CONTROL     "/api/buzzer/control"
#define API_PATH_LED_CONTROL        "/api/led/control"
#define API_PATH_WIFI_SCAN          "/api/wifi/scan"
#define API_PATH_WIFI_CONFIG        "/api/wifi/config"
#define API_PATH_DEVICE_INFO        "/api/device/info"

// LED控制宏定义（与main.c保持一致）
#define LED_PIN                    HI_IO_NAME_GPIO_2
#define LED_GPIO_FUN               HI_IO_FUNC_GPIO_2_GPIO
#define LED_ON()                   hi_gpio_set_ouput_val(LED_PIN, HI_GPIO_VALUE1)
#define LED_OFF()                  hi_gpio_set_ouput_val(LED_PIN, HI_GPIO_VALUE0)

// PHP API返回码
typedef enum {
    PHP_API_SUCCESS = 0,
    PHP_API_ERROR_GENERAL = -1,
    PHP_API_ERROR_INVALID_PARAM = -2,
    PHP_API_ERROR_SERVER_START_FAILED = -3,
    PHP_API_ERROR_DEVICE_CONTROL_FAILED = -4
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

// 蜂鸣器控制参数结构体
typedef struct {
    uint16_t duration_ms;     // 持续时间（毫秒）
    uint16_t on_time_ms;      // 开启时间（报警模式）
    uint16_t off_time_ms;     // 关闭时间（报警模式）
    uint16_t repeat_count;    // 重复次数
    uint8_t mode;            // 0:单次响, 1:报警模式
} buzzer_control_t;

// LED控制参数结构体
typedef struct {
    uint8_t state;           // 0:关闭, 1:开启, 2:闪烁
    uint16_t blink_interval; // 闪烁间隔（毫秒）
} led_control_t;

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
 * @brief 控制蜂鸣器
 * @param control 控制参数
 * @return php_api_result_t 控制结果
 */
php_api_result_t php_api_control_buzzer(const buzzer_control_t *control);

/**
 * @brief 控制LED
 * @param control 控制参数
 * @return php_api_result_t 控制结果
 */
php_api_result_t php_api_control_led(const led_control_t *control);

/**
 * @brief 获取实时传感器数据
 * @param temperature 温度指针
 * @param humidity 湿度指针
 * @return php_api_result_t 获取结果
 */
php_api_result_t php_api_get_sensor_data(uint8_t *temperature, uint8_t *humidity);

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

/**
 * @brief 解析JSON请求参数
 * @param request 请求字符串
 * @param key 参数键名
 * @param value 参数值指针
 * @param max_len 最大长度
 * @return int 解析结果（0成功，-1失败）
 */
int php_api_parse_json_param(const char *request, const char *key, char *value, size_t max_len);

/**
 * @brief 更新LED状态（用于闪烁控制）
 */
void php_api_led_update(void);

/**
 * @brief LED初始化函数
 */
void php_api_led_init(void);

#endif