/**
 ****************************************************************************************************
 * @file        php_api.c
 * @author      Hi3861 WiFi-IoT Project
 * @version     V1.0
 * @date        2024-12-19
 * @brief       PHP API模块实现 - HTTP服务器和Web接口
 * @license     Copyright (c) 2024, Hi3861 WiFi-IoT Project
 ****************************************************************************************************
 * @attention
 *
 * 实现HTTP服务器功能，支持PHP客户端API接口
 * 专注API服务，网络功能由network任务处理
 *
 */

#include "php_api.h"
#include <stdlib.h>

// 全局变量
static int g_server_socket = -1;
static bool g_server_running = false;
static osThreadId_t g_server_thread = NULL;

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
    
    // 处理API请求
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
        // 获取传感器数据
        php_api_send_response(client_socket, 200, "application/json",
            "{\"success\":true,\"message\":\"Sensor data\",\"data\":{\"temperature\":25,\"humidity\":60}}");
    }
    else if (strcmp(path, API_PATH_SYSTEM_INFO) == 0) {
        // 获取系统信息
        php_api_send_response(client_socket, 200, "application/json",
            "{\"success\":true,\"message\":\"System info\",\"data\":{\"device_name\":\"Hi3861\",\"version\":\"1.0.0\"}}");
    }
    else if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
        // 提供Web配置界面
        const char* html = 
            "<!DOCTYPE html>"
            "<html>"
            "<head>"
            "<meta charset=\"UTF-8\">"
            "<title>Hi3861配置界面</title>"
            "</head>"
            "<body>"
            "<h1>Hi3861 WiFi-IoT设备配置</h1>"
            "<p>设备状态: <span id=\"status\">获取中...</span></p>"
            "<button onclick=\"getStatus()\">刷新状态</button>"
            "<script>"
            "function getStatus() {"
            "  fetch('/api/status').then(r => r.json()).then(data => {"
            "    document.getElementById('status').innerHTML = JSON.stringify(data);"
            "  });"
            "}"
            "getStatus();"
            "</script>"
            "</body>"
            "</html>";
        php_api_send_response(client_socket, 200, "text/html; charset=utf-8", html);
    }
    else {
        php_api_send_response(client_socket, 404, "text/plain", "Not Found");
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
    else if (status_code == 500) status_text = "Internal Server Error";
    
    snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n",
        status_code, status_text, content_type, strlen(body));
    
    send(client_socket, header, strlen(header), 0);
    send(client_socket, body, strlen(body), 0);
}

/**
 * @brief 初始化PHP API模块
 */
php_api_result_t php_api_init(void) {
    log_i("PHP_API", "Initializing PHP API module...");
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
    strcpy(status->firmware_version, "1.0.0");
    status->uptime_ms = Time_GetCurrentMs();
    
    // 这里可以添加获取传感器数据的代码
    // status->temperature = g_temperature;
    // status->humidity = g_humidity;
    
    // 获取WiFi状态（由network任务处理）
    char* ip = WiFi_GetLocalIP();
    if (ip != NULL) {
        strcpy(status->ip_address, ip);
        strcpy(status->wifi_status, "Connected");
    } else {
        strcpy(status->ip_address, "0.0.0.0");
        strcpy(status->wifi_status, "Disconnected");
    }
    
    return PHP_API_SUCCESS;
}