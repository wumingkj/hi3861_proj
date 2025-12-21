/**
 ****************************************************************************************************
 * @file        php_api.c
 * @author      Hi3861 WiFi-IoT Project
 * @version     V2.0
 * @date        2024-12-19
 * @brief       PHP API模块实现 - HTTP服务器和纯网络API接口
 * @license     Copyright (c) 2024, Hi3861 WiFi-IoT Project
 ****************************************************************************************************
 * @attention
 *
 * 实现HTTP服务器功能，支持纯网络交互API接口
 * 移除网页内嵌，专注设备控制API
 *
 */

#include "php_api.h"
#include <stdlib.h>

// 全局变量
static int g_server_socket = -1;
static bool g_server_running = false;
static osThreadId_t g_server_thread = NULL;

// 外部全局变量声明（在main.c中定义）
extern uint8_t g_temperature;
extern uint8_t g_humidity;
extern bool g_wifi_connected;
extern char g_wifi_ssid[32];
extern char g_wifi_ip[16];

// LED控制相关变量
static uint8_t g_led_state = 0; // 0:关闭, 1:开启, 2:闪烁
static uint16_t g_led_blink_interval = 100; // 默认100ms闪烁间隔
static uint32_t g_last_led_update = 0;

/**
 * @brief HTTP服务器线程函数
 */
static void php_api_server_thread(void *arg) {
    (void)arg;
    
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    // 创建socket
    g_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (g_server_socket < 0) {
        log_e("PHP_API", "Failed to create socket");
        return;
    }
    
    // 设置socket选项
    int opt = 1;
    setsockopt(g_server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 绑定地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PHP_API_SERVER_PORT);
    
    if (bind(g_server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        log_e("PHP_API", "Failed to bind socket");
        close(g_server_socket);
        g_server_socket = -1;
        return;
    }
    
    // 监听连接
    if (listen(g_server_socket, PHP_API_MAX_CLIENTS) < 0) {
        log_e("PHP_API", "Failed to listen on socket");
        close(g_server_socket);
        g_server_socket = -1;
        return;
    }
    
    log_i("PHP_API", "HTTP server started on port %d", PHP_API_SERVER_PORT);
    g_server_running = true;
    
    // 接受客户端连接
    while (g_server_running) {
        int client_socket = accept(g_server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (g_server_running) {
                log_e("PHP_API", "Failed to accept client connection");
            }
            continue;
        }
        
        // 设置接收超时
        struct timeval timeout;
        timeout.tv_sec = PHP_API_REQUEST_TIMEOUT_MS / 1000;
        timeout.tv_usec = (PHP_API_REQUEST_TIMEOUT_MS % 1000) * 1000;
        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        
        log_i("PHP_API", "Client connected from %s", inet_ntoa(client_addr.sin_addr));
        
        // 处理客户端请求
        php_api_handle_client(client_socket);
        
        close(client_socket);
        log_i("PHP_API", "Client disconnected");
    }
    
    close(g_server_socket);
    g_server_socket = -1;
}

/**
 * @brief 解析JSON请求参数
 */
int php_api_parse_json_param(const char *request, const char *key, char *value, size_t max_len) {
    char search_pattern[64];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\":\"", key);
    
    const char *pos = strstr(request, search_pattern);
    if (pos == NULL) {
        // 尝试数字格式
        snprintf(search_pattern, sizeof(search_pattern), "\"%s\":", key);
        pos = strstr(request, search_pattern);
        if (pos == NULL) {
            return -1;
        }
        pos += strlen(search_pattern);
        
        // 解析数字
        const char *end = strchr(pos, ',');
        if (end == NULL) end = strchr(pos, '}');
        if (end == NULL) return -1;
        
        size_t len = end - pos;
        if (len >= max_len) len = max_len - 1;
        strncpy(value, pos, len);
        value[len] = '\0';
        return 0;
    }
    
    pos += strlen(search_pattern);
    const char *end = strchr(pos, '"');
    if (end == NULL) return -1;
    
    size_t len = end - pos;
    if (len >= max_len) len = max_len - 1;
    strncpy(value, pos, len);
    value[len] = '\0';
    
    return 0;
}

/**
 * @brief 处理HTTP客户端请求
 */
void php_api_handle_client(int client_socket) {
    char buffer[PHP_API_BUFFER_SIZE];
    ssize_t bytes_received;
    
    // 接收HTTP请求
    bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        log_e("PHP_API", "Failed to receive request");
        return;
    }
    
    buffer[bytes_received] = '\0';
    log_d("PHP_API", "Received request: %s", buffer);
    
    // 解析请求行
    char method[16], path[256], protocol[16];
    if (sscanf(buffer, "%15s %255s %15s", method, path, protocol) != 3) {
        php_api_send_response(client_socket, 400, "text/plain", "Bad Request");
        return;
    }
    
    log_i("PHP_API", "HTTP %s %s", method, path);
    
    // 处理API请求 - 纯网络交互API
    if (strcmp(path, API_PATH_STATUS) == 0) {
        system_status_t status;
        if (php_api_get_system_status(&status) == PHP_API_SUCCESS) {
            char json_buffer[512];
            snprintf(json_buffer, sizeof(json_buffer),
                "{\"success\":true,\"message\":\"System status\",\"data\":"
                "{\"device_name\":\"%s\",\"firmware_version\":\"%s\","
                "\"uptime_ms\":%u,\"temperature\":%d,\"humidity\":%d,"
                "\"wifi_status\":\"%s\",\"ip_address\":\"%s\"}}",
                status.device_name, status.firmware_version, status.uptime_ms,
                status.temperature, status.humidity, status.wifi_status, status.ip_address);
            php_api_send_response(client_socket, 200, "application/json", json_buffer);
        } else {
            php_api_send_response(client_socket, 500, "application/json", 
                "{\"success\":false,\"message\":\"Failed to get system status\"}");
        }
    }
    else if (strcmp(path, API_PATH_SENSOR_DATA) == 0) {
        // 获取实时传感器数据
        uint8_t temperature, humidity;
        if (php_api_get_sensor_data(&temperature, &humidity) == PHP_API_SUCCESS) {
            char json_buffer[256];
            snprintf(json_buffer, sizeof(json_buffer),
                "{\"success\":true,\"message\":\"Real-time sensor data\",\"data\":"
                "{\"temperature\":%d,\"humidity\":%d}}",
                temperature, humidity);
            php_api_send_response(client_socket, 200, "application/json", json_buffer);
        } else {
            php_api_send_response(client_socket, 500, "application/json",
                "{\"success\":false,\"message\":\"Failed to get sensor data\"}");
        }
    }
    else if (strcmp(path, API_PATH_BUZZER_CONTROL) == 0) {
        // 蜂鸣器控制API
        if (strcmp(method, "POST") == 0) {
            buzzer_control_t control = {0};
            char param_value[16];
            
            // 解析控制参数
            if (php_api_parse_json_param(buffer, "mode", param_value, sizeof(param_value)) == 0) {
                control.mode = atoi(param_value);
            }
            if (php_api_parse_json_param(buffer, "duration", param_value, sizeof(param_value)) == 0) {
                control.duration_ms = atoi(param_value);
            }
            if (php_api_parse_json_param(buffer, "on_time", param_value, sizeof(param_value)) == 0) {
                control.on_time_ms = atoi(param_value);
            }
            if (php_api_parse_json_param(buffer, "off_time", param_value, sizeof(param_value)) == 0) {
                control.off_time_ms = atoi(param_value);
            }
            if (php_api_parse_json_param(buffer, "repeat", param_value, sizeof(param_value)) == 0) {
                control.repeat_count = atoi(param_value);
            }
            
            php_api_result_t result = php_api_control_buzzer(&control);
            if (result == PHP_API_SUCCESS) {
                php_api_send_response(client_socket, 200, "application/json",
                    "{\"success\":true,\"message\":\"Buzzer control executed\"}");
            } else {
                php_api_send_response(client_socket, 500, "application/json",
                    "{\"success\":false,\"message\":\"Buzzer control failed\"}");
            }
        } else {
            php_api_send_response(client_socket, 405, "application/json",
                "{\"success\":false,\"message\":\"Method not allowed\"}");
        }
    }
    else if (strcmp(path, API_PATH_LED_CONTROL) == 0) {
        // LED控制API
        if (strcmp(method, "POST") == 0) {
            led_control_t control = {0};
            char param_value[16];
            
            // 解析控制参数
            if (php_api_parse_json_param(buffer, "state", param_value, sizeof(param_value)) == 0) {
                control.state = atoi(param_value);
            }
            if (php_api_parse_json_param(buffer, "interval", param_value, sizeof(param_value)) == 0) {
                control.blink_interval = atoi(param_value);
            }
            
            php_api_result_t result = php_api_control_led(&control);
            if (result == PHP_API_SUCCESS) {
                php_api_send_response(client_socket, 200, "application/json",
                    "{\"success\":true,\"message\":\"LED control executed\"}");
            } else {
                php_api_send_response(client_socket, 500, "application/json",
                    "{\"success\":false,\"message\":\"LED control failed\"}");
            }
        } else {
            php_api_send_response(client_socket, 405, "application/json",
                "{\"success\":false,\"message\":\"Method not allowed\"}");
        }
    }
    else if (strcmp(path, API_PATH_WIFI_SCAN) == 0) {
        // WiFi扫描API
        php_api_send_response(client_socket, 200, "application/json",
            "{\"success\":true,\"message\":\"WiFi scan initiated\",\"data\":{\"scanning\":true}}");
    }
    else if (strcmp(path, API_PATH_DEVICE_INFO) == 0) {
        // 设备信息API
        php_api_send_response(client_socket, 200, "application/json",
            "{\"success\":true,\"message\":\"Device information\",\"data\":"
            "{\"device\":\"Hi3861\",\"version\":\"2.0.0\",\"api_version\":\"pure_network\"}}");
    }
    else {
        // 返回API文档
        const char* api_doc = 
            "{\"success\":true,\"message\":\"Hi3861 Pure Network API\",\"apis\":["
            "{\"path\":\"/api/status\",\"method\":\"GET\",\"desc\":\"获取系统状态\"},"
            "{\"path\":\"/api/sensor/data\",\"method\":\"GET\",\"desc\":\"获取实时温湿度数据\"},"
            "{\"path\":\"/api/buzzer/control\",\"method\":\"POST\",\"desc\":\"控制蜂鸣器\"},"
            "{\"path\":\"/api/led/control\",\"method\":\"POST\",\"desc\":\"控制LED指示灯\"},"
            "{\"path\":\"/api/device/info\",\"method\":\"GET\",\"desc\":\"获取设备信息\"}"
            "]}";
        php_api_send_response(client_socket, 200, "application/json", api_doc);
    }
}

/**
 * @brief 发送HTTP响应
 */
void php_api_send_response(int client_socket, int status_code, const char *content_type, const char *body) {
    char header[512];
    const char* status_text = "OK";
    
    if (status_code == 404) status_text = "Not Found";
    else if (status_code == 400) status_text = "Bad Request";
    else if (status_code == 405) status_text = "Method Not Allowed";
    else if (status_code == 500) status_text = "Internal Server Error";
    
    snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "\r\n",
        status_code, status_text, content_type, strlen(body));
    
    send(client_socket, header, strlen(header), 0);
    send(client_socket, body, strlen(body), 0);
}

/**
 * @brief 初始化PHP API模块
 */
php_api_result_t php_api_init(void) {
    log_i("PHP_API", "Initializing PHP API module (Pure Network API v2.0)...");
    
    // 初始化LED控制状态
    g_led_state = 2; // 默认闪烁模式
    g_led_blink_interval = 100;
    g_last_led_update = Time_GetCurrentMs();
    
    log_i("PHP_API", "PHP API module initialized successfully");
    return PHP_API_SUCCESS;
}

/**
 * @brief 启动HTTP服务器
 */
php_api_result_t php_api_start_server(void) {
    if (g_server_running) {
        log_w("PHP_API", "HTTP server is already running");
        return PHP_API_SUCCESS;
    }
    
    // 创建服务器线程
    osThreadAttr_t attr = {
        .name = "PHP_API_Server",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U,
        .stack_mem = NULL,
        .stack_size = 8192,
        .priority = osPriorityNormal
    };
    
    g_server_thread = osThreadNew(php_api_server_thread, NULL, &attr);
    if (g_server_thread == NULL) {
        log_e("PHP_API", "Failed to create server thread");
        return PHP_API_ERROR_SERVER_START_FAILED;
    }
    
    log_i("PHP_API", "HTTP server thread created successfully");
    return PHP_API_SUCCESS;
}

/**
 * @brief 停止HTTP服务器
 */
php_api_result_t php_api_stop_server(void) {
    if (!g_server_running) {
        return PHP_API_SUCCESS;
    }
    
    g_server_running = false;
    
    // 关闭服务器socket以唤醒accept阻塞
    if (g_server_socket >= 0) {
        shutdown(g_server_socket, SHUT_RDWR);
        close(g_server_socket);
        g_server_socket = -1;
    }
    
    log_i("PHP_API", "HTTP server stopped");
    return PHP_API_SUCCESS;
}

/**
 * @brief 获取系统状态信息
 */
php_api_result_t php_api_get_system_status(system_status_t *status) {
    if (status == NULL) {
        return PHP_API_ERROR_INVALID_PARAM;
    }
    
    // 填充设备信息
    strcpy(status->device_name, "Hi3861_WiFi_IoT");
    strcpy(status->firmware_version, "2.0.0");
    status->uptime_ms = Time_GetCurrentMs();
    
    // 获取实时传感器数据
    status->temperature = g_temperature;
    status->humidity = g_humidity;
    
    // 获取WiFi状态
    char* ip = WiFi_GetLocalIP();
    if (ip != NULL) {
        strcpy(status->ip_address, ip);
        strcpy(status->wifi_status, g_wifi_connected ? "Connected" : "AP Mode");
    } else {
        strcpy(status->ip_address, "0.0.0.0");
        strcpy(status->wifi_status, "Disconnected");
    }
    
    return PHP_API_SUCCESS;
}

/**
 * @brief 控制蜂鸣器
 */
php_api_result_t php_api_control_buzzer(const buzzer_control_t *control) {
    if (control == NULL) {
        return PHP_API_ERROR_INVALID_PARAM;
    }
    
    log_i("PHP_API", "Buzzer control: mode=%d, duration=%d, on_time=%d, off_time=%d, repeat=%d",
          control->mode, control->duration_ms, control->on_time_ms, 
          control->off_time_ms, control->repeat_count);
    
    if (control->mode == 0) {
        // 单次响模式
        if (control->duration_ms > 0) {
            Buzzer_BeepMs(control->duration_ms);
            log_i("PHP_API", "Buzzer single beep: %d ms", control->duration_ms);
        }
    } else if (control->mode == 1) {
        // 报警模式
        if (control->on_time_ms > 0 && control->repeat_count > 0) {
            Buzzer_Alarm(control->repeat_count, control->on_time_ms, control->off_time_ms);
            log_i("PHP_API", "Buzzer alarm: %d times, on=%dms, off=%dms", 
                  control->repeat_count, control->on_time_ms, control->off_time_ms);
        }
    }
    
    return PHP_API_SUCCESS;
}

/**
 * @brief 控制LED
 */
php_api_result_t php_api_control_led(const led_control_t *control) {
    if (control == NULL) {
        return PHP_API_ERROR_INVALID_PARAM;
    }
    
    log_i("PHP_API", "LED control: state=%d, interval=%d", 
          control->state, control->blink_interval);
    
    g_led_state = control->state;
    if (control->blink_interval > 0) {
        g_led_blink_interval = control->blink_interval;
    }
    
    return PHP_API_SUCCESS;
}

/**
 * @brief 获取实时传感器数据
 */
php_api_result_t php_api_get_sensor_data(uint8_t *temperature, uint8_t *humidity) {
    if (temperature == NULL || humidity == NULL) {
        return PHP_API_ERROR_INVALID_PARAM;
    }
    
    // 直接返回全局变量中的实时数据
    *temperature = g_temperature;
    *humidity = g_humidity;
    
    return PHP_API_SUCCESS;
}

/**
 * @brief LED控制更新函数（需要在主循环中调用）
 */
void php_api_led_update(void) {
    uint32_t current_time = Time_GetCurrentMs();
    
    if (current_time - g_last_led_update >= g_led_blink_interval) {
        g_last_led_update = current_time;
        
        switch (g_led_state) {
            case 0: // 关闭
                LED_OFF();
                break;
            case 1: // 开启
                LED_ON();
                break;
            case 2: // 闪烁
                static uint8_t led_toggle = 0;
                if (led_toggle) {
                    LED_ON();
                } else {
                    LED_OFF();
                }
                led_toggle = !led_toggle;
                break;
        }
    }
}