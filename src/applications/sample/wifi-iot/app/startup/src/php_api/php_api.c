#include "php_api.h"

#include <ctype.h>  // 添加ctype.h头文件
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "hi_reset.h"  // 添加硬件重启头文件
#include "kv.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "time.h"

extern void Buzzer_Alarm(uint32_t duration_ms, uint32_t freq_hz, uint32_t volume);
extern char g_wifi_ip[16];  // 生成的代码片段

// 在全局变量定义前添加函数声明
static php_api_result_t php_api_handle_post_request(int client_socket, const char* request);
static php_api_result_t php_api_send_html_response(int client_socket);
static php_api_result_t php_api_send_success_response(int client_socket, const char* message);
static php_api_result_t php_api_send_error_response(int client_socket, const char* message);

// 日志输出频率控制
static uint32_t g_send_counter = 0;   // 发送计数器
static uint32_t g_log_interval = 10;  // 日志输出间隔（每10次发送记录一次日志）
static uint32_t g_success_count = 0;  // 成功发送计数
static uint32_t g_failure_count = 0;  // 失败发送计数
static bool g_php_api_initialized = false;
static char* g_json_buffer = NULL;
static uint32_t g_buffer_size = PHP_API_BUFFER_SIZE;
static int g_web_server_socket = -1;       // 网页服务器socket
static bool g_web_server_running = false;  // 网页服务器运行状态

// 默认系统状态
static php_api_system_status_t g_system_status = {.device_name = "",
                                                  .server_ip = "",
                                                  .local_ip = "",
                                                  .wifi_connected = false,
                                                  .wifi_ssid = "",
                                                  .user_id = "ap",
                                                  .bind_line = "",};

// HTML网页内容（包含系统配置和WiFi配置）
static const char* g_webpage_html =
    "<!DOCTYPE html>"
    "<html lang=\"zh-CN\">"
    "<head>"
    "    <meta charset=\"UTF-8\">"
    "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
    "    <title>设备配置页面</title>"
    "    <style>"
    "        body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }"
    "        .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; "
    "box-shadow: 0 2px 4px rgba(0,0,0,0.1); }"
    "        h1 { color: #333; text-align: center; }"
    "        h2 { color: #555; border-bottom: 1px solid #ddd; padding-bottom: 10px; }"
    "        .form-group { margin-bottom: 15px; }"
    "        label { display: block; margin-bottom: 5px; font-weight: bold; }"
    "        input[type=\"text\"], input[type=\"password\"] { width: 100%; padding: 8px; border: 1px solid #ddd; "
    "border-radius: 4px; box-sizing: border-box; }"
    "        button { background-color: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 4px; "
    "cursor: pointer; font-size: 16px; margin-right: 10px; }"
    "        button:hover { background-color: #0056b3; }"
    "        .status { margin-top: 20px; padding: 10px; border-radius: 4px; }"
    "        .success { background-color: #d4edda; color: #155724; border: 1px solid #c3e6cb; }"
    "        .error { background-color: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }"
    "        .section { margin-bottom: 30px; }"
    "    </style>"
    "</head>"
    "<body>"
    "    <div class=\"container\">"
    "        <h1>设备配置页面</h1>"
    "        <form id=\"configForm\" method=\"post\">"
    "            <div class=\"section\">"
    "                <h2>系统配置</h2>"
    "                <div class=\"form-group\">"
    "                    <label for=\"device_name\">设备名称:</label>"
    "                    <input type=\"text\" id=\"device_name\" name=\"device_name\" value=\"普中hi3861\" required>"
    "                </div>"
    "                <div class=\"form-group\">"
    "                    <label for=\"server_ip\">服务器IP:</label>"
    "                    <input type=\"text\" id=\"server_ip\" name=\"server_ip\" value=\"192.168.15.99\" placeholder=\"192.168.0.1\" "
    "required>"
    "                </div>"
    "                <div class=\"form-group\">"
    "                    <label for=\"user_id\">用户ID:</label>"
    "                    <input type=\"text\" id=\"user_id\" name=\"user_id\" value=\"ap\" required>"
    "                </div>"
    "                <div class=\"form-group\">"
    "                    <label for=\"bind_line\">生产线:</label>"
    "                    <input type=\"text\" id=\"bind_line\" name=\"bind_line\" value=\"1001\" required>"
    "                </div>"
    "            </div>"
    "            <div class=\"section\">"
    "                <h2>WiFi配置</h2>"
    "                <div class=\"form-group\">"
    "                    <label for=\"wifi_ssid\">WiFi名称 (SSID):</label>"
    "                    <input type=\"text\" id=\"wifi_ssid\" name=\"wifi_ssid\" value=\"喵\" placeholder=\"输入WiFi名称\" "
    "required>"
    "                </div>"
    "                <div class=\"form-group\">"
    "                    <label for=\"wifi_password\">WiFi密码:</label>"
    "                    <input type=\"password\" id=\"wifi_password\" name=\"wifi_password\" value=\"aojunjie\""
    "placeholder=\"输入WiFi密码\">"
    "                </div>"
    "                <div class=\"form-group\">"
    "                    <label for=\"wifi_auth_type\">加密方式:</label>"
    "                    <input type=\"text\" id=\"wifi_auth_type\" name=\"wifi_auth_type\" value=\"WPA2\" "
    "placeholder=\"WPA2/WPA/WEP/开放网络\">"
    "                </div>"
    "            </div>"
    "            <button type=\"submit\">保存所有配置</button>"
    "            <button type=\"button\" onclick=\"loadCurrentConfig()\">加载当前配置</button>"
    "        </form>"
    "        <div id=\"statusMessage\"></div>"
    "    </div>"
    "    <script>"
    "        // 提交表单处理"
    "        document.getElementById('configForm').addEventListener('submit', function(e) {"
    "            e.preventDefault();"
    "            const formData = new FormData(this);"
    "            fetch('/', {"
    "                method: 'POST',"
    "                body: new URLSearchParams(formData)"
    "            })"
    "            .then(response => response.text())"
    "            .then(data => {"
    "                const statusDiv = document.getElementById('statusMessage');"
    "                if (data.includes('保存成功')) {"
    "                    statusDiv.innerHTML = '<div class=\"status success\">' + data + '</div>';"
    "                } else {"
    "                    statusDiv.innerHTML = '<div class=\"status error\">' + data + '</div>';"
    "                }"
    "            })"
    "            .catch(error => {"
    "                document.getElementById('statusMessage').innerHTML = '<div class=\"status error\">提交失败: ' + "
    "error + '</div>';"
    "            });"
    "        });"
    "        "
    "        // 加载当前配置（从KV存储读取）"
    "        function loadCurrentConfig() {"
    "            // 这里可以添加从设备获取当前配置的逻辑"
    "            alert('加载当前配置功能需要设备支持');"
    "        }"
    "    </script>"
    "</body>"
    "</html>";

/**
 * @brief 初始化PHP API模块
 */
php_api_result_t php_api_init(uint32_t buffer_size) {
    if (g_php_api_initialized) {
        log_w("PHP_API", "模块已初始化");
        return PHP_API_SUCCESS;
    }

    // 设置缓冲区大小
    if (buffer_size > 0) {
        g_buffer_size = buffer_size;
    }

    // 分配JSON缓冲区
    g_json_buffer = (char*)malloc(g_buffer_size);
    if (g_json_buffer == NULL) {
        log_e("PHP_API", "JSON缓冲区分配失败");
        return PHP_API_ERROR_MEMORY_ALLOC;
    }

    // 从KV存储读取系统状态
    php_api_get_system_status(&g_system_status);

    g_php_api_initialized = true;
    log_i("PHP_API", "PHP API模块初始化成功，缓冲区大小: %u", g_buffer_size);

    return PHP_API_SUCCESS;
}

/**
 * @brief 释放PHP API模块资源
 */
void php_api_deinit(void) {
    if (!g_php_api_initialized) {
        return;
    }

    // 释放JSON缓冲区
    if (g_json_buffer != NULL) {
        free(g_json_buffer);
        g_json_buffer = NULL;
    }

    g_php_api_initialized = false;
    log_i("PHP_API", "PHP API模块资源已释放");
}

/**
 * @brief 获取系统状态信息（从KV存储读取所有配置）
 */
php_api_result_t php_api_get_system_status(php_api_system_status_t* status) {
    if (status == NULL) {
        return PHP_API_ERROR_INVALID_PARAM;
    }

    // 初始化结构体
    memset(status, 0, sizeof(php_api_system_status_t));

    // 直接读取到结构体字段中
    if (kv_get_string("device_name", status->device_name, sizeof(status->device_name)) == KV_SUCCESS) {
        // 读取成功
    }

    if (kv_get_string("server_ip", status->server_ip, sizeof(status->server_ip)) == KV_SUCCESS) {
        // 读取成功
    }

    if (kv_get_string("user_id", status->user_id, sizeof(status->user_id)) == KV_SUCCESS) {
        // 读取成功
    }

    if (kv_get_string("bind_line", status->bind_line, sizeof(status->bind_line)) == KV_SUCCESS) {
        // 读取成功
    }

    if (g_wifi_ip[0] != '\0') {
        strncpy(status->local_ip, g_wifi_ip, sizeof(status->local_ip) - 1);
    }

    status->wifi_connected = php_api_is_wifi_connected();

    log_i("PHP_API", "系统状态读取成功: 设备=%s, 服务器IP=%s", status->device_name, status->server_ip);

    return PHP_API_SUCCESS;
}

/**
 * @brief 保存系统状态信息到KV存储
 */
php_api_result_t php_api_save_system_status(const php_api_system_status_t* status) {
    if (status == NULL) {
        return PHP_API_ERROR_INVALID_PARAM;
    }

    // 保存设备名称
    if (kv_set_string("device_name", status->device_name) != KV_SUCCESS) {
        log_e("PHP_API", "保存设备名称失败");
        return PHP_API_ERROR_MEMORY_ALLOC;
    }

    // 保存服务器IP
    if (kv_set_string("server_ip", status->server_ip) != KV_SUCCESS) {
        log_e("PHP_API", "保存服务器IP失败");
        return PHP_API_ERROR_MEMORY_ALLOC;
    }

    // 保存用户ID
    if (kv_set_string("user_id", status->user_id) != KV_SUCCESS) {
        log_e("PHP_API", "保存用户ID失败");
        return PHP_API_ERROR_MEMORY_ALLOC;
    }

    // 保存生产线
    if (kv_set_string("bind_line", status->bind_line) != KV_SUCCESS) {
        log_e("PHP_API", "保存生产线失败");
        return PHP_API_ERROR_MEMORY_ALLOC;
    }

    log_i("PHP_API", "系统状态保存成功: 设备=%s, 服务器IP=%s", status->device_name, status->server_ip);

    return PHP_API_SUCCESS;
}

/**
 * @brief 发送握手信息到服务器
 */
php_api_result_t php_api_send_handshake(void) {
    // 获取系统状态
    php_api_system_status_t system_status;
    php_api_get_system_status(&system_status);

    // 检查服务器IP是否设置
    if (strlen(system_status.server_ip) == 0) {
        log_e("PHP_API", "服务器IP未设置");
        return PHP_API_ERROR_SERVER_IP_NOT_SET;
    }

    // 检查WiFi连接状态
    if (!php_api_is_wifi_connected()) {
        log_e("PHP_API", "WiFi未连接");
        return PHP_API_ERROR_WIFI_NOT_CONNECTED;
    }

    // 构建握手JSON数据
    char* json_buffer = (char*)malloc(512);
    if (json_buffer == NULL) {
        log_e("PHP_API", "握手JSON缓冲区分配失败");
        return PHP_API_ERROR_MEMORY_ALLOC;
    }

    int json_len = snprintf(json_buffer, 512,
                            "{\"type\":\"handshake\","
                            "\"device_name\":\"%s\","
                            "\"local_ip\":\"%s\","
                            "\"user_id\":\"%s\","
                            "\"bind_line\":\"%s\","
                            "\"running_status\":%d}",
                            system_status.device_name, system_status.local_ip, system_status.user_id,
                            system_status.bind_line, g_running_status);

    if (json_len >= 512) {
        log_e("PHP_API", "握手JSON数据过长");
        free(json_buffer);
        return PHP_API_ERROR_MEMORY_ALLOC;
    }

    // 发送握手数据
    php_api_result_t result = php_api_send_json_data(json_buffer);
    free(json_buffer);

    if (result == PHP_API_SUCCESS) {
        log_i("PHP_API", "握手信息发送成功");
    } else {
        log_e("PHP_API", "握手信息发送失败");
    }

    return result;
}

/**
 * @brief 发送传感器数据到服务器
 */
php_api_result_t php_api_send_sensor_data(const sensor_data_t* sensor_data) {
    if (sensor_data == NULL) {
        log_e("PHP_API", "传感器数据为空");
        return PHP_API_ERROR_INVALID_PARAM;
    }

    // 获取系统状态
    php_api_system_status_t system_status;
    php_api_get_system_status(&system_status);

    // 检查服务器IP是否设置
    if (strlen(system_status.server_ip) == 0) {
        log_e("PHP_API", "服务器IP未设置");
        return PHP_API_ERROR_SERVER_IP_NOT_SET;
    }

    // 检查WiFi连接状态
    if (!php_api_is_wifi_connected()) {
        log_e("PHP_API", "WiFi未连接");
        return PHP_API_ERROR_WIFI_NOT_CONNECTED;
    }

    // 构建传感器数据JSON
    char* json_buffer = (char*)malloc(1024);
    if (json_buffer == NULL) {
        log_e("PHP_API", "传感器JSON缓冲区分配失败");
        return PHP_API_ERROR_MEMORY_ALLOC;
    }

    int json_len =
        snprintf(json_buffer, 1024,
                 "{\"type\":\"sensor_data\","
                 "\"device_name\":\"%s\","
                 "\"local_ip\":\"%s\","
                 "\"user_id\":\"%s\","
                 "\"bind_line\":\"%s\","
                 "\"running_status\":%d,"
                 "\"temperature\":%.2f,"
                 "\"humidity\":%.2f,"
                 "\"smoke\":%.2f,"
                 "\"relay_status\":%s,"
                 "\"buzzer_status\":%s}",
                 system_status.device_name, system_status.local_ip, system_status.user_id, system_status.bind_line,
                 g_running_status, sensor_data->temperature, sensor_data->humidity, sensor_data->smoke,
                 sensor_data->relay_status ? "true" : "false", sensor_data->buzzer_status ? "true" : "false");

    if (json_len >= 1024) {
        log_e("PHP_API", "传感器JSON数据过长");
        free(json_buffer);
        return PHP_API_ERROR_MEMORY_ALLOC;
    }

    // 发送传感器数据
    php_api_result_t result = php_api_send_json_data(json_buffer);
    free(json_buffer);

    if (result == PHP_API_SUCCESS) {
        log_i("PHP_API", "传感器数据发送成功");
    } else {
        log_e("PHP_API", "传感器数据发送失败");
    }

    return result;
}

/**
 * @brief 发送JSON数据到服务器（核心发送函数）
 */
php_api_result_t php_api_send_json_data(const char* json_data) {
    if (json_data == NULL) {
        log_e("PHP_API", "JSON数据为空");
        return PHP_API_ERROR_INVALID_PARAM;
    }

    // 获取系统状态
    php_api_system_status_t system_status;
    php_api_get_system_status(&system_status);

    // 检查服务器IP是否设置
    if (strlen(system_status.server_ip) == 0) {
        log_e("PHP_API", "服务器IP未设置");
        return PHP_API_ERROR_SERVER_IP_NOT_SET;
    }

    // 创建socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        log_e("PHP_API", "创建socket失败");
        return PHP_API_ERROR_SOCKET_FAILED;
    }

    // 设置超时
    struct timeval send_timeout;
    send_timeout.tv_sec = PHP_API_SEND_TIMEOUT_MS / 1000;
    send_timeout.tv_usec = (PHP_API_SEND_TIMEOUT_MS % 1000) * 1000;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &send_timeout, sizeof(send_timeout));

    // 设置服务器地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PHP_API_SERVER_PORT);

    if (inet_pton(AF_INET, system_status.server_ip, &server_addr.sin_addr) <= 0) {
        log_e("PHP_API", "无效的服务器IP地址: %s", system_status.server_ip);
        close(sockfd);
        return PHP_API_ERROR_SERVER_IP_NOT_SET;
    }

    // 连接到服务器
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        log_e("PHP_API", "连接到服务器失败: %s:%d", system_status.server_ip, PHP_API_SERVER_PORT);
        close(sockfd);
        return PHP_API_ERROR_CONNECT_FAILED;
    }

    // 连接成功日志（按间隔输出）
    g_send_counter++;
    if (g_send_counter % g_log_interval == 0) {
        log_i("PHP_API", "成功连接到服务器: %s:%d (第%d次发送)", system_status.server_ip, PHP_API_SERVER_PORT,
              g_send_counter);
    }

    // 构建HTTP POST请求
    size_t json_length = strlen(json_data);
    size_t http_request_size = json_length + 512;  // HTTP头大约512字节

    char* http_request = (char*)malloc(http_request_size);
    if (http_request == NULL) {
        log_e("PHP_API", "HTTP请求缓冲区分配失败");
        close(sockfd);
        return PHP_API_ERROR_MEMORY_ALLOC;
    }

    int request_len = snprintf(http_request, http_request_size,
                               "POST /setInfo.php HTTP/1.1\r\n"
                               "Host: %s:%d\r\n"
                               "Content-Type: application/json\r\n"
                               "Content-Length: %zu\r\n"
                               "Connection: close\r\n"
                               "\r\n"
                               "%s",
                               system_status.server_ip, PHP_API_SERVER_PORT, json_length, json_data);

    if (request_len >= http_request_size) {
        log_e("PHP_API", "HTTP请求缓冲区不足");
        free(http_request);
        close(sockfd);
        return PHP_API_ERROR_MEMORY_ALLOC;
    }

    // 发送HTTP请求
    ssize_t bytes_sent = send(sockfd, http_request, request_len, 0);
    free(http_request);

    if (bytes_sent < 0) {
        log_e("PHP_API", "发送HTTP请求失败");
        close(sockfd);
        return PHP_API_ERROR_SEND_FAILED;
    }

    // 发送成功日志（按间隔输出）
    if (g_send_counter % g_log_interval == 0) {
        log_i("PHP_API", "HTTP请求发送成功，发送字节数: %d (第%d次发送)", (int)bytes_sent, g_send_counter);
    }

    // 接收服务器响应（改进版：解决截断问题）
    char* response_buffer = (char*)malloc(4096);  // 增大缓冲区到4KB
    if (response_buffer == NULL) {
        log_e("PHP_API", "响应缓冲区分配失败");
        close(sockfd);
        return PHP_API_ERROR_MEMORY_ALLOC;
    }

    size_t total_received = 0;
    size_t buffer_size = 4096;

    // 设置接收超时（3秒）
    struct timeval recv_timeout;
    recv_timeout.tv_sec = 3;
    recv_timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&recv_timeout, sizeof(recv_timeout));

    // 循环接收完整响应（改进版）
    int max_attempts = 10;  // 最多尝试10次
    int attempts = 0;

    while (total_received < buffer_size - 1 && attempts < max_attempts) {
        ssize_t bytes_received = recv(sockfd, response_buffer + total_received, buffer_size - total_received - 1, 0);

        if (bytes_received > 0) {
            total_received += bytes_received;
            attempts = 0;  // 重置尝试次数

            // 检查是否收到完整的HTTP响应（包含空行分隔符）
            if (strstr(response_buffer, "\r\n\r\n") != NULL) {
                // 查找Content-Length头
                char* content_length_start = strstr(response_buffer, "Content-Length: ");
                if (content_length_start != NULL) {
                    int content_length = atoi(content_length_start + 16);
                    char* body_start = strstr(response_buffer, "\r\n\r\n");
                    if (body_start != NULL) {
                        body_start += 4;  // 跳过\r\n\r\n
                        size_t body_length = total_received - (body_start - response_buffer);

                        // 如果已经收到完整响应体，则退出循环
                        if (body_length >= (size_t)content_length) {
                            log_i("PHP_API", "收到完整响应，Content-Length: %d, 实际接收: %d", content_length,
                                  (int)body_length);
                            break;
                        }
                    }
                } else {
                    // 如果没有Content-Length，检查是否以空行结束
                    char* last_newline = strstr(response_buffer + total_received - 10, "\r\n");
                    if (last_newline && (last_newline - response_buffer) >= (ssize_t)(total_received - 4)) {
                        // 可能是没有Content-Length的响应，检查是否以空行结束
                        break;
                    }
                }
            }
        } else if (bytes_received == 0) {
            // 连接关闭
            log_i("PHP_API", "服务器关闭连接");
            break;
        } else {
            // 接收错误或超时
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 超时，等待一下再试
                attempts++;
                usleep(100000);  // 等待100ms
                continue;
            } else {
                log_e("PHP_API", "接收错误: %d", errno);
                break;
            }
        }
    }

    if (total_received > 0) {
        response_buffer[total_received] = '\0';

        // 调试信息：显示实际接收长度
        log_i("PHP_API", "实际接收字节数: %d", (int)total_received);

        // 服务器响应日志（直接输出完整内容，不拆分）
        if (g_send_counter % g_log_interval == 0) {
            log_i_long("PHP_API", "服务器响应 (总长度:%d, 第%d次发送): %s", (int)total_received, g_send_counter,
                       response_buffer);
        }
    } else {
        log_w("PHP_API", "未收到服务器响应");
    }

    free(response_buffer);
    close(sockfd);

    // 统计发送结果
    if (bytes_sent > 0) {
        g_success_count++;
    } else {
        g_failure_count++;
    }

    return PHP_API_SUCCESS;
}

/**
 * @brief 处理HTTP请求
 */
php_api_result_t php_api_handle_http_request(int client_socket) {
    // 动态分配请求缓冲区
    char* request_buffer = (char*)malloc(4096);
    if (request_buffer == NULL) {
        log_e("PHP_API", "请求缓冲区分配失败");
        return PHP_API_ERROR_MEMORY_ALLOC;
    }

    int bytes_received = recv(client_socket, request_buffer, 4095, 0);

    if (bytes_received <= 0) {
        log_e("PHP_API", "接收HTTP请求失败");
        free(request_buffer);
        return PHP_API_ERROR_SOCKET_FAILED;
    }

    request_buffer[bytes_received] = '\0';
    log_i("PHP_API", "收到HTTP请求: %s", request_buffer);

    php_api_result_t result;
    // 检查是否为POST请求（配置提交）
    if (strstr(request_buffer, "POST") != NULL) {
        result = php_api_handle_post_request(client_socket, request_buffer);
    } else {
        // 发送HTML页面
        result = php_api_send_html_response(client_socket);
    }

    free(request_buffer);
    return result;
}

/**
 * @brief 处理POST请求（配置保存）
 */
static php_api_result_t php_api_handle_post_request(int client_socket, const char* request) {
    // 解析POST数据
    const char* post_data_start = strstr(request, "\r\n\r\n");
    if (post_data_start == NULL) {
        return php_api_send_error_response(client_socket, "无效的POST请求");
    }

    post_data_start += 4;  // 跳过\r\n\r\n

    // 动态分配POST数据缓冲区
    char* post_data = (char*)malloc(1024);
    if (post_data == NULL) {
        return php_api_send_error_response(client_socket, "内存分配失败");
    }

    strncpy(post_data, post_data_start, 1023);
    post_data[1023] = '\0';

    log_i("PHP_API", "POST数据: %s", post_data);

    // 解析参数
    php_api_system_status_t new_status;
    memset(&new_status, 0, sizeof(new_status));

    char wifi_ssid[64] = {0};
    char wifi_password[64] = {0};
    char wifi_auth_type[32] = {0};

    char* token = strtok(post_data, "&");
    while (token != NULL) {
        char* equals = strchr(token, '=');
        if (equals != NULL) {
            *equals = '\0';
            char* key = token;
            char* value = equals + 1;

            // URL解码（简化版）- 动态分配解码缓冲区
            char* decoded_value = (char*)malloc(256);
            if (decoded_value != NULL) {
                php_api_url_decode(value, decoded_value, 255);

                if (strcmp(key, "device_name") == 0) {
                    strncpy(new_status.device_name, decoded_value, sizeof(new_status.device_name) - 1);
                } else if (strcmp(key, "server_ip") == 0) {
                    strncpy(new_status.server_ip, decoded_value, sizeof(new_status.server_ip) - 1);
                } else if (strcmp(key, "user_id") == 0) {
                    strncpy(new_status.user_id, decoded_value, sizeof(new_status.user_id) - 1);
                } else if (strcmp(key, "bind_line") == 0) {
                    strncpy(new_status.bind_line, decoded_value, sizeof(new_status.bind_line) - 1);
                } else if (strcmp(key, "wifi_ssid") == 0) {
                    strncpy(wifi_ssid, decoded_value, sizeof(wifi_ssid) - 1);
                } else if (strcmp(key, "wifi_password") == 0) {
                    strncpy(wifi_password, decoded_value, sizeof(wifi_password) - 1);
                } else if (strcmp(key, "wifi_auth_type") == 0) {
                    strncpy(wifi_auth_type, decoded_value, sizeof(wifi_auth_type) - 1);
                }

                free(decoded_value);
            }
        }
        token = strtok(NULL, "&");
    }

    free(post_data);

    // 验证必填字段
    if (strlen(new_status.server_ip) == 0) {
        return php_api_send_error_response(client_socket, "服务器IP不能为空");
    }

    // 保存系统配置到KV存储
    php_api_result_t result = php_api_save_system_status(&new_status);
    if (result != PHP_API_SUCCESS) {
        return php_api_send_error_response(client_socket, "保存系统配置失败");
    }

    // 保存WiFi配置到KV存储
    if (strlen(wifi_ssid) > 0) {
        if (kv_set_string("wifi_ssid", wifi_ssid) != KV_SUCCESS) {
            log_e("PHP_API", "保存WiFi SSID失败");
        } else {
            log_i("PHP_API", "WiFi SSID保存成功: %s", wifi_ssid);
        }

        if (strlen(wifi_password) > 0) {
            if (kv_set_string("wifi_password", wifi_password) != KV_SUCCESS) {
                log_e("PHP_API", "保存WiFi密码失败");
            } else {
                log_i("PHP_API", "WiFi密码保存成功,密码长度%d", strlen(wifi_password));
            }
        }

        if (strlen(wifi_auth_type) > 0) {
            if (kv_set_string("wifi_auth_type", wifi_auth_type) != KV_SUCCESS) {
                log_e("PHP_API", "保存WiFi加密方式失败");
            } else {
                log_i("PHP_API", "WiFi加密方式保存成功: %s", wifi_auth_type);
            }
        }
    }

    // 更新系统状态
    memcpy(&g_system_status, &new_status, sizeof(g_system_status));
    // 启动提示音
    Buzzer_Alarm(2, 50, 100);
    php_api_send_success_response(client_socket, "所有配置保存成功！设备将重新启动以应用WiFi配置。");

    Time_DelayMs(1000);
    // 执行系统重启
    RebootSystem();
    return php_api_send_success_response(client_socket, "如果您看到这条消息，证明重启失败，需要您手动复位");
}

/**
 * @brief 发送HTML页面响应（动态缓冲区版本）
 */
static php_api_result_t php_api_send_html_response(int client_socket) {
    // 计算需要的缓冲区大小（HTML页面长度 + HTTP头长度 + 安全余量）
    size_t html_length = strlen(g_webpage_html);
    size_t header_length = 200;                               // HTTP头大约长度
    size_t total_needed = html_length + header_length + 100;  // 增加100字节安全余量

    // 动态分配缓冲区
    char* response = (char*)malloc(total_needed);
    if (response == NULL) {
        log_e("PHP_API", "HTML响应缓冲区分配失败，需要: %zu 字节", total_needed);
        return php_api_send_error_response(client_socket, "服务器内部错误：内存不足");
    }

    int response_len = snprintf(response, total_needed,
                                "HTTP/1.1 200 OK\r\n"
                                "Content-Type: text/html; charset=utf-8\r\n"
                                "Content-Length: %zu\r\n"
                                "Connection: close\r\n"
                                "\r\n"
                                "%s",
                                html_length, g_webpage_html);

    if (response_len >= total_needed) {
        log_e("PHP_API", "HTTP响应缓冲区不足，需要: %d 字节，实际: %zu 字节", response_len, total_needed);
        free(response);
        return php_api_send_error_response(client_socket, "服务器内部错误：缓冲区不足");
    }

    if (send(client_socket, response, response_len, 0) < 0) {
        log_e("PHP_API", "发送HTML响应失败");
        free(response);
        return PHP_API_ERROR_SEND_FAILED;
    }

    log_i("PHP_API", "HTML页面发送成功，响应长度: %d 字节", response_len);
    free(response);
    return PHP_API_SUCCESS;
}

/**
#endif
 * @brief 发送成功响应（动态缓冲区版本）
 */
static php_api_result_t php_api_send_success_response(int client_socket, const char* message) {
    // 动态计算缓冲区大小
    size_t message_length = strlen(message);
    size_t total_needed = message_length + 200;  // 消息长度 + HTTP头长度
    char* response = (char*)malloc(total_needed);
    if (response == NULL) {
        log_e("PHP_API", "成功响应缓冲区分配失败");
        return PHP_API_ERROR_MEMORY_ALLOC;
    }

    int response_len = snprintf(response, total_needed,
                                "HTTP/1.1 200 OK\r\n"
                                "Content-Type: text/plain; charset=utf-8\r\n"
                                "Content-Length: %zu\r\n"
                                "Connection: close\r\n"
                                "\r\n"
                                "%s",
                                message_length, message);

    if (send(client_socket, response, response_len, 0) < 0) {
        log_e("PHP_API", "发送成功响应失败");
        free(response);
        return PHP_API_ERROR_SEND_FAILED;
    }
    log_i("PHP_API", "成功响应发送: %s", message);
    free(response);
    return PHP_API_SUCCESS;
}

/**
 * @brief 发送错误响应（动态缓冲区版本）
 */
static php_api_result_t php_api_send_error_response(int client_socket, const char* message) {
    // 动态计算缓冲区大小
    size_t message_length = strlen(message);
    size_t total_needed = message_length + 200;  // 消息长度 + HTTP头长度

    char* response = (char*)malloc(total_needed);
    if (response == NULL) {
        log_e("PHP_API", "错误响应缓冲区分配失败");
        return PHP_API_ERROR_MEMORY_ALLOC;
    }

    int response_len = snprintf(response, total_needed,
                                "HTTP/1.1 400 Bad Request\r\n"
                                "Content-Type: text/plain; charset=utf-8\r\n"
                                "Content-Length: %zu\r\n"
                                "Connection: close\r\n"
                                "\r\n"
                                "%s",
                                message_length, message);

    if (send(client_socket, response, response_len, 0) < 0) {
        log_e("PHP_API", "发送错误响应失败");
        free(response);
        return PHP_API_ERROR_SEND_FAILED;
    }

    log_e("PHP_API", "错误响应发送: %s", message);
    free(response);
    return PHP_API_SUCCESS;
}

/**
 * @brief URL解码（动态缓冲区版本）
 */
static void php_api_url_decode(const char* src, char* dst, size_t dst_size) {
    size_t i = 0, j = 0;
    while (src[i] != '\0' && j < dst_size - 1) {
        if (src[i] == '%' && isxdigit(src[i + 1]) && isxdigit(src[i + 2])) {
            char hex[3] = {src[i + 1], src[i + 2], '\0'};
            dst[j++] = (char)strtol(hex, NULL, 16);
            i += 3;
        } else if (src[i] == '+') {
            dst[j++] = ' ';
            i++;
        } else {
            dst[j++] = src[i++];
        }
    }
    dst[j] = '\0';
}

/**
 * @brief 获取WiFi连接状态
 */
bool php_api_is_wifi_connected(void) {
    // 这里需要根据实际的WiFi API来获取连接状态
    // 暂时返回true表示已连接
    return true;
}

/**
 * @brief 停止网页服务器
 */
void php_api_stop_web_server(void) {
    if (g_web_server_running) {
        g_web_server_running = false;

        if (g_web_server_socket >= 0) {
            close(g_web_server_socket);
            g_web_server_socket = -1;
        }

        log_i("PHP_API", "网页服务器已停止");
    }
}

/**
 * @brief 启动HTTP网页服务器
 */
php_api_result_t php_api_start_web_server(void) {
    if (!g_php_api_initialized) {
        log_e("PHP_API", "PHP API模块未初始化");
        return PHP_API_ERROR_INIT_FAILED;
    }

    if (g_web_server_running) {
        log_w("PHP_API", "网页服务器已在运行");
        return PHP_API_SUCCESS;
    }

    // 创建socket
    g_web_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (g_web_server_socket < 0) {
        log_e("PHP_API", "创建socket失败");
        return PHP_API_ERROR_SOCKET_FAILED;
    }

    // 设置socket选项，允许地址重用
    int opt = 1;
    if (setsockopt(g_web_server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        log_w("PHP_API", "设置socket选项失败，但继续运行");
    }

    // 绑定地址和端口
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PHP_API_SERVER_PORT);

    if (bind(g_web_server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        log_e("PHP_API", "绑定端口%d失败", PHP_API_SERVER_PORT);
        close(g_web_server_socket);
        g_web_server_socket = -1;
        return PHP_API_ERROR_SOCKET_FAILED;
    }

    // 开始监听
    if (listen(g_web_server_socket, 5) < 0) {
        log_e("PHP_API", "监听失败");
        close(g_web_server_socket);
        g_web_server_socket = -1;
        return PHP_API_ERROR_SOCKET_FAILED;
    }

    g_web_server_running = true;
    log_i("PHP_API", "HTTP网页服务器启动成功，监听端口: %d", PHP_API_SERVER_PORT);

    return PHP_API_SUCCESS;
}

/**
 * @brief 更新HTTP网页服务器（处理客户端连接）
 */
php_api_result_t php_api_update_web_server(void) {
    if (!g_web_server_running || g_web_server_socket < 0) {
        return PHP_API_ERROR_INIT_FAILED;
    }

    // 检查是否有客户端连接
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_socket = accept(g_web_server_socket, (struct sockaddr*)&client_addr, &client_len);

    if (client_socket < 0) {
        // 没有客户端连接，不是错误
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return PHP_API_SUCCESS;
        }
        log_e("PHP_API", "接受客户端连接失败");
        return PHP_API_ERROR_SOCKET_FAILED;
    }

    // 处理HTTP请求
    php_api_result_t result = php_api_handle_http_request(client_socket);

    // 关闭客户端连接
    close(client_socket);

    return result;
}