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

// 全局变量 - 新增UDP广播相关
static int g_udp_socket = -1;
static bool g_udp_running = false;
static osThreadId_t g_udp_thread = NULL;


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
 * @brief 解析JSON请求参数（增强版，支持PHP前端格式）
 */
int php_api_parse_json_param(const char *request, const char *key, char *value, size_t max_len) {
    char search_pattern[64];
    
    // 先查找JSON请求体（POST请求）
    const char *json_start = strstr(request, "\r\n\r\n");
    if (json_start == NULL) {
        json_start = request; // 如果没有找到分隔符，使用整个请求
    } else {
        json_start += 4; // 跳过\r\n\r\n
    }
    
    // 尝试字符串格式 "\"key\":\"value\""
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\":\"", key);
    const char *pos = strstr(json_start, search_pattern);
    if (pos != NULL) {
        pos += strlen(search_pattern);
        const char *end = strchr(pos, '"');
        if (end != NULL) {
            size_t len = end - pos;
            if (len >= max_len) len = max_len - 1;
            strncpy(value, pos, len);
            value[len] = '\0';
            return 0;
        }
    }
    
    // 尝试数字格式 "\"key\":value"
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\":", key);
pos = strstr(json_start, search_pattern);
    if (pos != NULL) {
        pos += strlen(search_pattern);
        const char *end = strchr(pos, ',');
        if (end == NULL) end = strchr(pos, '}');
        if (end == NULL) end = strchr(pos, '\r');
        if (end == NULL) end = strchr(pos, '\n');
        if (end == NULL) return -1;
        
        size_t len = end - pos;
        if (len >= max_len) len = max_len - 1;
        strncpy(value, pos, len);
        value[len] = '\0';
        return 0;
    }
    
    // 尝试URL参数格式（GET请求）
    snprintf(search_pattern, sizeof(search_pattern), "%s=", key);
    pos = strstr(request, search_pattern);
    if (pos != NULL) {
        pos += strlen(search_pattern);
        const char *end = strchr(pos, '&');
        if (end == NULL) end = strchr(pos, ' ');
        if (end == NULL) end = strchr(pos, '\r');
        if (end == NULL) end = strchr(pos, '\n');
        if (end == NULL) return -1;
        
        size_t len = end - pos;
        if (len >= max_len) len = max_len - 1;
        strncpy(value, pos, len);
        value[len] = '\0';
        return 0;
    }
    
    return -1;
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
    
    // 处理API请求 - 适配PHP前端
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
    else if (strcmp(path, API_PATH_SENSOR_DATA) == 0 || 
             strcmp(path, API_PATH_SENSOR_TEMPERATURE) == 0 || 
             strcmp(path, API_PATH_SENSOR_HUMIDITY) == 0) {
        // 获取实时传感器数据 - 支持多个路径
        uint8_t temperature, humidity;
        if (php_api_get_sensor_data(&temperature, &humidity) == PHP_API_SUCCESS) {
            char json_buffer[256];
            
            if (strcmp(path, API_PATH_SENSOR_TEMPERATURE) == 0) {
                // 仅温度数据
                snprintf(json_buffer, sizeof(json_buffer),
                    "{\"success\":true,\"message\":\"Temperature data\",\"data\":"
                    "{\"temperature\":%d}}", temperature);
            } else if (strcmp(path, API_PATH_SENSOR_HUMIDITY) == 0) {
                // 仅湿度数据
                snprintf(json_buffer, sizeof(json_buffer),
                    "{\"success\":true,\"message\":\"Humidity data\",\"data\":"
                    "{\"humidity\":%d}}", humidity);
            } else {
                // 完整传感器数据
                snprintf(json_buffer, sizeof(json_buffer),
                    "{\"success\":true,\"message\":\"Real-time sensor data\",\"data\":"
                    "{\"temperature\":%d,\"humidity\":%d}}",
                    temperature, humidity);
            }
            php_api_send_response(client_socket, 200, "application/json", json_buffer);
        } else {
            php_api_send_response(client_socket, 500, "application/json",
                "{\"success\":false,\"message\":\"Failed to get sensor data\"}");
        }
    }
    else if (strcmp(path, API_PATH_BUZZER_CONTROL) == 0) {
        // 蜂鸣器控制API - 适配PHP前端格式
        if (strcmp(method, "POST") == 0) {
            buzzer_control_t control = {0};
            char param_value[16];
            
// 解析PHP前端格式的控制参数
            if (php_api_parse_json_param(buffer, "action", param_value, sizeof(param_value)) == 0) {
                // 根据action参数设置模式
                if (strcmp(param_value, "beep") == 0) {
                    control.mode = 0;
                    control.duration_ms = 100; // 默认100ms
                } else if (strcmp(param_value, "alert") == 0) {
                    control.mode = 1;
                    control.on_time_ms = 200;
                    control.off_time_ms = 200;
                    control.repeat_count = 3;
                } else if (strcmp(param_value, "test") == 0) {
                    control.mode = 1;
                    control.on_time_ms = 100;
                    control.off_time_ms = 100;
                    control.repeat_count = 5;
                } else if (strcmp(param_value, "custom") == 0) {
                    control.mode = 0;
                    if (php_api_parse_json_param(buffer, "duration", param_value, sizeof(param_value)) == 0) {
                        control.duration_ms = atoi(param_value);
                    } else {
                        control.duration_ms = 100; // 默认值
                    }
                }
            }
            
// 解析自定义参数
            if (php_api_parse_json_param(buffer, "frequency", param_value, sizeof(param_value)) == 0) {
                // 频率参数（虽然当前蜂鸣器驱动不支持频率调节，但保留参数）
                log_i("PHP_API", "Buzzer frequency: %s Hz", param_value);
            }
            if (php_api_parse_json_param(buffer, "duration", param_value, sizeof(param_value)) == 0) {
                control.duration_ms = atoi(param_value);
            }
            
            php_api_result_t result = php_api_control_buzzer(&control);
            if (result == PHP_API_SUCCESS) {
                php_api_send_response(client_socket, 200, "application/json",
                    "{\"success\":true,\"message\":\"Buzzer control executed\",\"data\":{\"action\":\"executed\"}}");
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
        // LED控制API - 适配PHP前端格式
        if (strcmp(method, "POST") == 0) {
            led_control_t control = {0};
            char param_value[16];
            
            // 解析PHP前端格式的控制参数
            if (php_api_parse_json_param(buffer, "state", param_value, sizeof(param_value)) == 0) {
                // PHP前端使用字符串"0"/"1"，转换为数字
                if (strcmp(param_value, "0") == 0) control.state = 0;
                else if (strcmp(param_value, "1") == 0) control.state = 1;
                else control.state = atoi(param_value);
            }
            
            php_api_result_t result = php_api_control_led(&control);
            if (result == PHP_API_SUCCESS) {
                php_api_send_response(client_socket, 200, "application/json",
                    "{\"success\":true,\"message\":\"LED control executed\",\"data\":{\"state\":\"executed\"}}");
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
    else if (strcmp(path, API_PATH_WIFI_CONFIG) == 0) {
        // WiFi配置API - 使用KV存储保存配置
        if (strcmp(method, "POST") == 0) {
            char wifi_ssid[64] = "";
            char wifi_password[64] = "";
            kv_result_t result;
            
            // 解析PHP前端格式的WiFi配置参数
            if (php_api_parse_json_param(buffer, "ssid", wifi_ssid, sizeof(wifi_ssid)) == 0 &&
                php_api_parse_json_param(buffer, "password", wifi_password, sizeof(wifi_password)) == 0) {
                
                // 验证参数有效性
                if (strlen(wifi_ssid) == 0 || strlen(wifi_password) == 0) {
                    php_api_send_response(client_socket, 400, "application/json",
                        "{\"success\":false,\"message\":\"SSID or password cannot be empty\"}");
                    return;
                }
                
                // 使用KV存储保存WiFi配置
                result = kv_set_string("wifi_ssid", wifi_ssid);
                if (result != KV_SUCCESS) {
                    php_api_send_response(client_socket, 500, "application/json",
                        "{\"success\":false,\"message\":\"Failed to save SSID to KV storage\"}");
                    return;
                }
                
                result = kv_set_string("wifi_password", wifi_password);
                if (result != KV_SUCCESS) {
                    php_api_send_response(client_socket, 500, "application/json",
                        "{\"success\":false,\"message\":\"Failed to save password to KV storage\"}");
                    return;
                }
                
                log_i("PHP_API", "WiFi配置已保存: SSID=%s, Password=***", wifi_ssid);
                
                // 返回成功响应
                php_api_send_response_formatted(client_socket, 200, "application/json",
                    "{\"success\":true,\"message\":\"WiFi configuration saved successfully\",\"data\":"
                    "{\"ssid\":\"%s\",\"saved\":true,\"reboot_required\":true}}", wifi_ssid);
            } else {
                php_api_send_response(client_socket, 400, "application/json",
                    "{\"success\":false,\"message\":\"Missing required parameters: ssid and password\"}");
            }
        } else if (strcmp(method, "GET") == 0) {
            // 获取已保存的WiFi配置
            char saved_ssid[64] = "";
            kv_result_t result;
            
            result = kv_get_string("wifi_ssid", saved_ssid, sizeof(saved_ssid));
            if (result == KV_SUCCESS) {
                // 密码不返回给客户端，只返回配置状态
                php_api_send_response_formatted(client_socket, 200, "application/json",
                    "{\"success\":true,\"message\":\"WiFi configuration retrieved\",\"data\":"
                    "{\"ssid\":\"%s\",\"configured\":true}}", saved_ssid);
            } else {
                php_api_send_response(client_socket, 200, "application/json",
                    "{\"success\":true,\"message\":\"No WiFi configuration found\",\"data\":"
                    "{\"configured\":false}}");
            }
        } else {
            php_api_send_response(client_socket, 405, "application/json",
                "{\"success\":false,\"message\":\"Method not allowed\"}");
        }
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
            "{\"path\":\"/api/sensor/temperature\",\"method\":\"GET\",\"desc\":\"仅获取温度数据\"},"
            "{\"path\":\"/api/sensor/humidity\",\"method\":\"GET\",\"desc\":\"仅获取湿度数据\"},"
            "{\"path\":\"/api/buzzer/control\",\"method\":\"POST\",\"desc\":\"控制蜂鸣器\"},"
            "{\"path\":\"/api/led/control\",\"method\":\"POST\",\"desc\":\"控制LED指示灯\"},"
            "{\"path\":\"/api/wifi/scan\",\"method\":\"GET\",\"desc\":\"扫描WiFi网络\"},"
            "{\"path\":\"/api/wifi/config\",\"method\":\"POST\",\"desc\":\"配置WiFi网络\"},"
            "{\"path\":\"/api/device/info\",\"method\":\"GET\",\"desc\":\"获取设备信息\"}"
            "]}";
        php_api_send_response(client_socket, 200, "application/json", api_doc);
    }
} // 添加这个结束括号

/**
 * @brief 发送HTTP响应（支持格式化字符串）
 */
void php_api_send_response_formatted(int client_socket, int status_code, const char *content_type, const char *format, ...) {
    char body[512];
    char header[512];
    const char* status_text = "OK";
    va_list args;
    
    if (status_code == 404) status_text = "Not Found";
    else if (status_code == 400) status_text = "Bad Request";
    else if (status_code == 405) status_text = "Method Not Allowed";
    else if (status_code == 500) status_text = "Internal Server Error";
    
    // 格式化响应体
    va_start(args, format);
    vsnprintf(body, sizeof(body), format, args);
    va_end(args);
    
    // 构建HTTP头部
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
 * @brief 发送HTTP响应
 */
void php_api_send_response(int client_socket, int status_code, const char *content_type, const char *body) {
    php_api_send_response_formatted(client_socket, status_code, content_type, "%s", body);
}

/**
 * @brief LED初始化函数
 */
void php_api_led_init(void) {
    hi_gpio_init();                                            // GPIO初始化
    hi_io_set_pull(LED_PIN, HI_IO_PULL_DOWN);                  // 设置GPIO下拉
    hi_io_set_func(LED_PIN, LED_GPIO_FUN);                     // 设置IO为GPIO功能
    hi_gpio_set_dir(LED_PIN, HI_GPIO_DIR_OUT);                 // 设置GPIO为输出模式
    log_i("PHP_API", "LED初始化完成");
}

/**
 * @brief 初始化PHP API模块
 */
php_api_result_t php_api_init(void) {
    log_i("PHP_API", "Initializing PHP API module (Pure Network API v2.0)...");
    
    // 初始化LED
    php_api_led_init();
    
    // 初始化LED控制状态
    g_led_state = 0; // 默认闪烁模式
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
 * @brief 控制蜂鸣器（适配PHP前端）
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
        } else {
            Buzzer_BeepMs(100); // 默认100ms
        }
    } else if (control->mode == 1) {
        // 报警模式
        if (control->on_time_ms > 0 && control->repeat_count > 0) {
            Buzzer_Alarm(control->repeat_count, control->on_time_ms, control->off_time_ms);
            log_i("PHP_API", "Buzzer alarm: %d times, on=%dms, off=%dms", 
                  control->repeat_count, control->on_time_ms, control->off_time_ms);
        } else {
            Buzzer_Alarm(3, 200, 200); // 默认报警模式
        }
    }
    
    return PHP_API_SUCCESS;
}

/**
 * @brief 控制LED（适配PHP前端）
 */
php_api_result_t php_api_control_led(const led_control_t *control) {
    if (control == NULL) {
        return PHP_API_ERROR_INVALID_PARAM;
    }
    
    log_i("PHP_API", "LED control: state=%d", control->state);
    
    g_led_state = control->state;
    
    // 立即更新LED状态（仅对非闪烁模式）
    if (g_led_state == 0) {
        LED_OFF();
    } else if (g_led_state == 1) {
        LED_ON();
    }
    // state=2 闪烁模式，由php_api_led_update函数处理
    
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
    
    // 仅在闪烁模式下进行周期性更新
    if (g_led_state == 2) {
        if (current_time - g_last_led_update >= g_led_blink_interval) {
            g_last_led_update = current_time;
            
            static uint8_t led_toggle = 0;
            
            if (led_toggle) {
                LED_ON();
            } else {
                LED_OFF();
            }
            led_toggle = !led_toggle;
        }
    }
    // 对于非闪烁模式（关闭/开启），仅在状态改变时更新，不进行周期性更新
}

/**
 * @brief UDP广播监听线程函数
 */
static void php_api_udp_broadcast_thread(void *arg) {
    (void)arg;
    
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    // 创建UDP socket
    g_udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_udp_socket < 0) {
        log_e("PHP_API", "Failed to create UDP socket");
        return;
    }
    
    // 设置socket选项 - 允许广播
    int opt = 1;
    setsockopt(g_udp_socket, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
    setsockopt(g_udp_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 绑定地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PHP_API_UDP_RECEIVE_PORT);
    
    if (bind(g_udp_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        log_e("PHP_API", "Failed to bind UDP socket on port %d", PHP_API_UDP_RECEIVE_PORT);
        close(g_udp_socket);
        g_udp_socket = -1;
        return;
    }
    
    // 设置接收超时
    struct timeval timeout;
    timeout.tv_sec = PHP_API_UDP_TIMEOUT_MS / 1000;
    timeout.tv_usec = (PHP_API_UDP_TIMEOUT_MS % 1000) * 1000;
    setsockopt(g_udp_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    log_i("PHP_API", "UDP broadcast listener started on port %d", PHP_API_UDP_RECEIVE_PORT);
    g_udp_running = true;
    
    // 监听UDP广播
    while (g_udp_running) {
        char buffer[PHP_API_UDP_BUFFER_SIZE];
        ssize_t bytes_received;
        
        bytes_received = recvfrom(g_udp_socket, buffer, sizeof(buffer) - 1, 0, 
                                 (struct sockaddr*)&client_addr, &client_len);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            log_i("PHP_API", "Received UDP message from %s:%d (%d bytes)", 
                  inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), bytes_received);
            
            // 处理UDP消息
            php_api_handle_udp_discovery(&client_addr, buffer, bytes_received);
        } else if (bytes_received < 0) {
            if (g_udp_running) {
                // 超时是正常的，继续监听
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    log_e("PHP_API", "UDP recvfrom error: %d", errno);
                }
            }
        }
    }
    
    close(g_udp_socket);
    g_udp_socket = -1;
    log_i("PHP_API", "UDP broadcast listener stopped");
}

/**
 * @brief 处理UDP广播发现请求
 */
void php_api_handle_udp_discovery(struct sockaddr_in *client_addr, const char *message, int message_len) {
    // 解析JSON消息
    char message_type[32] = "";
    char request_id[32] = "";
    
    // 尝试解析JSON消息
    if (php_api_parse_json_param(message, "type", message_type, sizeof(message_type)) == 0) {
        if (strcmp(message_type, "discovery") == 0) {
            // 发现请求
            php_api_parse_json_param(message, "request_id", request_id, sizeof(request_id));
            log_i("PHP_API", "Handling discovery request from %s, request_id: %s", 
                  inet_ntoa(client_addr->sin_addr), request_id);
            
            // 发送响应
            php_api_send_udp_response(client_addr, request_id);
        }
    } else {
        // 如果不是JSON格式，检查是否是简单的发现消息
        if (strstr(message, "discovery") != NULL || strstr(message, "DISCOVERY") != NULL) {
            log_i("PHP_API", "Handling simple discovery request from %s", 
                  inet_ntoa(client_addr->sin_addr));
            php_api_send_udp_response(client_addr, "simple_request");
        }
    }
}

/**
 * @brief 发送UDP发现响应
 */
void php_api_send_udp_response(struct sockaddr_in *client_addr, const char *request_id) {
    char response[512];
    char* ip = WiFi_GetLocalIP();
    
    // 构建响应消息
    snprintf(response, sizeof(response),
        "{\"type\":\"discovery_response\",\"request_id\":\"%s\",\"timestamp\":%u,"
        "\"device_name\":\"Hi3861_WiFi_IoT\",\"ip_address\":\"%s\",\"http_port\":%d,"
        "\"firmware_version\":\"2.0.0\",\"api_version\":\"pure_network\"}",
        request_id, (unsigned int)time(NULL), ip ? ip : "0.0.0.0", PHP_API_SERVER_PORT);
    
    // 发送响应到客户端
    ssize_t sent = sendto(g_udp_socket, response, strlen(response), 0,
                         (struct sockaddr*)client_addr, sizeof(*client_addr));
    
    if (sent > 0) {
        log_i("PHP_API", "Sent discovery response to %s:%d (%d bytes)", 
              inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port), sent);
    } else {
        log_e("PHP_API", "Failed to send discovery response");
    }
}

/**
 * @brief 启动UDP广播监听服务
 */
php_api_result_t php_api_start_udp_broadcast(void) {
    if (g_udp_running) {
        log_w("PHP_API", "UDP broadcast listener is already running");
        return PHP_API_SUCCESS;
    }
    
    // 创建UDP广播线程
    osThreadAttr_t attr = {
        .name = "PHP_API_UDP",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U,
        .stack_mem = NULL,
        .stack_size = 4096,
        .priority = osPriorityNormal
    };
    
    g_udp_thread = osThreadNew(php_api_udp_broadcast_thread, NULL, &attr);
    if (g_udp_thread == NULL) {
        log_e("PHP_API", "Failed to create UDP broadcast thread");
        return PHP_API_ERROR_SERVER_START_FAILED;
    }
    
    log_i("PHP_API", "UDP broadcast thread created successfully");
    return PHP_API_SUCCESS;
}

/**
 * @brief 停止UDP广播监听服务
 */
php_api_result_t php_api_stop_udp_broadcast(void) {
    if (!g_udp_running) {
        return PHP_API_SUCCESS;
    }
    
    g_udp_running = false;
    
    // 关闭UDP socket以唤醒recvfrom阻塞
    if (g_udp_socket >= 0) {
        shutdown(g_udp_socket, SHUT_RDWR);
        close(g_udp_socket);
        g_udp_socket = -1;
    }
    
    log_i("PHP_API", "UDP broadcast listener stopped");
    return PHP_API_SUCCESS;
}