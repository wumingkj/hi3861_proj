#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "hi_io.h"
#include "hi_gpio.h"
#include "buzzer.h"
#include "dht11.h"
#include "time.h"        // 使用新的时间库
#include "wifi.h"        // 修改为相对路径
#include "kv.h"          // 新增KV存储模块头文件
#include "php_api.h"     // 恢复PHP API模块头文件
#include "key_manager.h" // 新增按键管理器头文件
#include "smoke.h"
#include "nfc.h"
#include "NT3H.h"
// #define DEFAULT_DEBUG_LEVEL 0
#include "debug.h" // 新增调试头文件

/*
#        ┏┓　　　┏┓+ +
#　　　┏┛┻━━━┛┻┓ + +
#　　　┃　　　　　　　┃ 　
#　　　┃　　　━　　　┃ ++ + + +
#　　 ████━████ ┃+
#　　　┃　　　　　　　┃ +
#　　　┃　　　┻　　　┃
#　　　┃　　　　　　　┃ + +
#　　　┗━┓　　　┏━┛
#　　　　　┃　　　┃　　　　　　　　　　　
#　　　　　┃　　　┃ + + + +
#　　　　　┃　　　┃　　　　Codes are far away from bugs with the animal protecting　　　
#　　　　　┃　　　┃ + 　　　　神兽保佑,代码无bug　　
#　　　　　┃　　　┃
#　　　　　┃　　　┃　　+　　　　　　　　　
#　　　　　┃　 　　┗━━━┓ + +
#　　　　　┃ 　　　　　　　┣┓
#　　　　　┃ 　　　　　　　┏┛
#　　　　　┗┓┓┏━┳┓┏┛ + + + +
#　　　　　　┃┫┫　┃┫┫
#　　　　　　┗┻┛　┗┻┛+ + + +
*/

// 函数原型声明
void led_init(void);
void network_task(void *arg); // 修改为合并后的网络任务声明
// 删除 php_api_task 声明

// 管脚定义
#define LED_PIN HI_IO_NAME_GPIO_2
#define LED_GPIO_FUN HI_IO_FUNC_GPIO_2_GPIO

// 按键引脚定义
#define KEY1_PIN HI_IO_NAME_GPIO_13
#define KEY2_PIN HI_IO_NAME_GPIO_12
// 删除LED控制宏定义（已在php_api.h中定义）
#define LED_ON() hi_gpio_set_ouput_val(LED_PIN, HI_GPIO_VALUE0)
#define LED_OFF() hi_gpio_set_ouput_val(LED_PIN, HI_GPIO_VALUE1)

osTimerId_t g_buzzer_tick_timer = NULL;
osTimerId_t g_key_timer = NULL; // 新增按键定时器

// 全局按键管理器
key_manager_t *g_key_manager = NULL;

// 全局变量用于存储传感器数据
uint8_t g_temperature = 255; // 改为uint8_t类型
uint8_t g_humidity = 255;    // 改为uint8_t类型
bool g_dht11_connected = true;

// 网络状态全局变量
bool g_wifi_connected = false;
char g_wifi_ssid[32] = "";
char g_wifi_ip[16] = "";

// 烟雾报警器相关变量 - 新增缺失的变量定义
bool g_smoke_alarm_triggered = false;
smoke_sensor_data_t g_smoke_data = {0};

// 烟雾传感器更新间隔常量
#define SMOKE_UPDATE_INTERVAL_MS 5000 // 5秒读取一次

uint8_t led_value = 0;

// 常量定义（毫秒）
#define SENSOR_UPDATE_INTERVAL_MS 2000  // 2秒读取一次
#define PHP_UPDATE_INTERVAL_MS 50       // 25ms-php-api更新
#define NETWORK_CHECK_INTERVAL_MS 10000 // 10秒检查一次网络状态

// DHT11读取重试机制
#define DHT11_RETRY_COUNT 3 // 每次读取最多重试3次
void BuzzerTickCb(void *arg)
{
    (void)arg;
    Buzzer_Tick(Time_GetCurrentMs());
}

// 按键事件处理函数
static void handle_key_event(uint8_t key_index, key_event_t event)
{
    switch (event)
    {
    case KEY_EVENT_PRESS:
        log_i("KEY", "按键%d按下", key_index + 1);
        break;
    case KEY_EVENT_RELEASE:
        log_i("KEY", "按键%d释放", key_index + 1);
        break;

    case KEY_EVENT_SHORT_PRESS:
        log_i("KEY", "按键%d短按", key_index + 1);
        // 按键1短按：切换LED状态
        if (key_index == 0)
        {
            led_value = !led_value;
            if (led_value)
            {
                LED_ON();
                log_i("KEY", "LED打开");
            }
            else
            {
                LED_OFF();
                log_i("KEY", "LED关闭");
            }
        }
        // 按键2短按：触发蜂鸣器提示音
        if (key_index == 1)
        {
            Buzzer_Alarm(1, 100, 0);
            log_i("KEY", "蜂鸣器提示音触发");
        }
        break;

    case KEY_EVENT_LONG_PRESS:
        log_i("KEY", "按键%d长按", key_index + 1);
        // 按键1长按：显示系统状态
        if (key_index == 0)
        {
            log_i("STATUS", "=== 系统状态 ===");
            if (g_wifi_connected)
            {
                log_i("STATUS", "WiFi: 已连接 %s", g_wifi_ssid);
                log_i("STATUS", "IP地址: %s", g_wifi_ip);
            }
            else
            {
                log_i("STATUS", "WiFi: AP模式运行中");
                log_i("STATUS", "IP地址: %s", g_wifi_ip);
            }
            if (g_dht11_connected)
            {
                log_i("STATUS", "温度: %d°C, 湿度: %d%%", g_temperature, g_humidity);
            }
            else
            {
                log_i("STATUS", "DHT11: 未连接");
            }
            log_i("STATUS", "=================");
        }
        // 按键2长按：触发蜂鸣器报警
        if (key_index == 1)
        {
            Buzzer_Alarm(3, 200, 100);
            log_i("KEY", "蜂鸣器报警触发");
        }
        break;

    case KEY_EVENT_DOUBLE_CLICK:
        log_i("KEY", "按键%d双击", key_index + 1);
        // 按键1双击：LED闪烁
        if (key_index == 0)
        {
            for (int i = 0; i < 3; i++)
            {
                LED_ON();
                Time_DelayMs(200);
                LED_OFF();
                Time_DelayMs(200);
            }
            log_i("KEY", "LED闪烁完成");
        }
        // 按键2双击：烟雾传感器测试
        if (key_index == 1)
        {
            log_i("SMOKE_TEST", "=== 烟雾传感器测试 ===");
            smoke_sensor_test();
            log_i("SMOKE_TEST", "====================");
        }
        break;

        // 按键2双击：显示按键状态
        if (key_index == 1)
        {
            log_i("KEY", "=== 按键状态 ===");
            for (uint8_t i = 0; i < g_key_manager->key_count; i++)
            {
                bool pressed = key_manager_is_pressed(g_key_manager, i);
                uint32_t duration = key_manager_get_press_duration(g_key_manager, i);
                log_i("KEY", "按键%d: %s, 持续时间: %dms",
                      i + 1, pressed ? "按下" : "释放", duration);
            }
            log_i("KEY", "================");
        }
        break;

    case KEY_EVENT_HOLD:
        // 保持按下状态，可以用于实现持续功能（如音量调节等）
        // 这里不记录日志，避免频繁输出
        break;

    default:
        break;
    }
}

// 按键定时器回调函数
void KeyTimerCb(void *arg)
{
    (void)arg;

    if (g_key_manager != NULL)
    {
        // 更新按键状态
        key_manager_update(g_key_manager);

        // 检查所有按键的事件
        for (uint8_t i = 0; i < g_key_manager->key_count; i++)
        {
            key_event_t event = key_manager_get_event(g_key_manager, i);
            if (event != KEY_EVENT_NONE)
            {
                handle_key_event(i, event);
            }
        }
    }
}

// 网络任务函数 - 使用KV存储系统
void network_task(void *arg)
{
    (void)arg;

    log_i("NETWORK", "网络任务开始启动");

    // 等待系统初始化完成
    sleep(2);
    log_i("NETWORK", "系统初始化完成，开始WiFi配置");

    // 从KV存储读取WiFi配置
    char wifi_ssid[32] = "";
    char wifi_password[64] = "";

    // 尝试从KV存储读取WiFi配置
    if (kv_get_string("wifi_ssid", wifi_ssid, sizeof(wifi_ssid)) == KV_SUCCESS &&
        kv_get_string("wifi_password", wifi_password, sizeof(wifi_password)) == KV_SUCCESS)
    {

        log_i("NETWORK", "从KV存储读取到WiFi配置: %s", wifi_ssid);

        // 尝试连接STA网络
        log_i("NETWORK", "正在连接到WiFi网络: %s", wifi_ssid);
        WifiErrorCode wifi_ret = WiFi_connectHotspots(wifi_ssid, wifi_password);

        if (wifi_ret == WIFI_SUCCESS)
        {
            log_i("NETWORK", "WiFi连接成功");
            g_wifi_connected = true;
            strncpy(g_wifi_ssid, wifi_ssid, sizeof(g_wifi_ssid) - 1);

            // 获取IP地址
            char *ip_addr = WiFi_GetLocalIP();
            if (ip_addr != NULL)
            {
                strncpy(g_wifi_ip, ip_addr, sizeof(g_wifi_ip) - 1);
                log_i("NETWORK", "获取到IP地址: %s", g_wifi_ip);
            }
        }
        else
        {
            log_w("NETWORK", "WiFi连接失败，错误码: %d", wifi_ret);
        }
    }
    else
    {
        log_i("NETWORK", "KV存储中没有找到WiFi配置");
    }

    // 如果STA连接失败或没有配置，创建AP模式
    if (!g_wifi_connected)
    {
        log_i("NETWORK", "启动AP模式作为备用方案");
        const char *ap_ssid = "Hi3861_Config_AP";
        const char *ap_password = "12345678";

        WifiErrorCode ap_ret = WiFi_createHotspots(ap_ssid, ap_password);
        if (ap_ret == WIFI_SUCCESS)
        {
            log_i("NETWORK", "AP热点创建成功: %s", ap_ssid);

            // 获取AP的IP地址
            char *ip_addr = WiFi_GetLocalIP();
            if (ip_addr != NULL)
            {
                strncpy(g_wifi_ip, ip_addr, sizeof(g_wifi_ip) - 1);
                log_i("NETWORK", "AP热点IP地址: %s", g_wifi_ip);
            }
        }
        else
        {
            log_e("NETWORK", "AP热点创建失败，错误码: %d", ap_ret);
        }
    }

    // 等待网络稳定
    sleep(2);

    // 初始化PHP API模块
    log_i("NETWORK", "正在初始化PHP API模块（纯网络API v2.0）...");
    php_api_result_t php_ret = php_api_init();
    if (php_ret != PHP_API_SUCCESS)
    {
        log_e("NETWORK", "PHP API初始化失败，错误码: %d", php_ret);
    }
    else
    {
        log_i("NETWORK", "PHP API初始化成功");
    }

    // 启动HTTP服务器
    log_i("NETWORK", "正在启动HTTP服务器...");
    php_ret = php_api_start_server();
    if (php_ret == PHP_API_SUCCESS)
    {
        log_i("NETWORK", "HTTP服务器启动成功");
    }
    else
    {
        log_w("NETWORK", "HTTP服务器启动失败，错误码: %d", php_ret);
    }

    log_i("NETWORK", "网络任务初始化完成，进入主循环");

    // 合并后的网络任务主循环
    static uint32_t g_last_network_check = 0; // 修复：使用0初始化
    static uint32_t g_last_php_update = 0;    // 修复：使用0初始化
    while (1)
    {
        uint32_t current_time_ms = Time_GetCurrentMs();

        // 如果是第一次运行，初始化时间戳
        if (g_last_network_check == 0)
        {
            g_last_network_check = current_time_ms;
        }
        if (g_last_php_update == 0)
        {
            g_last_php_update = current_time_ms;
        }

        // 网络状态检查（10秒间隔）
        if (current_time_ms - g_last_network_check >= NETWORK_CHECK_INTERVAL_MS)
        {
            g_last_network_check = current_time_ms;

            if (g_wifi_connected)
            {
                log_i("NETWORK", "WiFi状态: 已连接 %s, IP: %s", g_wifi_ssid, g_wifi_ip);
            }
            else
            {
                log_i("NETWORK", "网络状态: AP模式运行中, IP: %s", g_wifi_ip);
            }
        }

        // PHP API更新（50ms间隔）
        if (current_time_ms - g_last_php_update >= PHP_UPDATE_INTERVAL_MS)
        {
            g_last_php_update = current_time_ms;
            php_api_led_update();
        }

        // 添加短延时，让出CPU时间
        Time_DelayMs(10);
    }
}

// LED初始化函数 - 设置初始状态为关闭
void led_init(void)
{
    hi_io_set_func(LED_PIN, LED_GPIO_FUN);
    hi_gpio_set_dir(LED_PIN, HI_GPIO_DIR_OUT);
    LED_OFF(); // 初始状态设置为关闭
    log_i("LED", "LED初始化完成，初始状态：关闭");
}

// NFC检测相关变量
bool g_nfc_initialized = false;
uint32_t g_last_nfc_check = 0;
#define NFC_CHECK_INTERVAL_MS 30000 // 30秒检测一次NFC标签

// WIFI配置解析函数 - 添加清除NDEF数据功能
// WIFI配置解析函数 - 修改为在成功写入KV存储后清除NDEF数据
static void parse_wifi_config(const char *wifi_uri)
{
    char ssid[64] = {0};
    char password[64] = {0};
    char auth_type[32] = {0};

    // 跳过"WIFI:"前缀
    const char *ptr = wifi_uri + 5;

    while (*ptr != '\0')
    {
        // 查找分隔符
        if (*ptr == ';')
        {
            ptr++;
            continue;
        }

        // 解析字段
        if (*ptr == 'S' && *(ptr + 1) == ':')
        {
            // 解析SSID
            const char *start = ptr + 2;
            int i = 0;
            while (*start != ';' && *start != '\0')
            {
                ssid[i++] = *start++;
            }
            ssid[i] = '\0';
            log_i("NFC", "检测到SSID: %s", ssid);
            ptr = start;
        }
        else if (*ptr == 'T' && *(ptr + 1) == ':')
        {
            // 解析认证类型
            const char *start = ptr + 2;
            int i = 0;
            while (*start != ';' && *start != '\0')
            {
                auth_type[i++] = *start++;
            }
            auth_type[i] = '\0';
            log_i("NFC", "认证类型: %s", auth_type);
            ptr = start;
        }
        else if (*ptr == 'P' && *(ptr + 1) == ':')
        {
            // 解析密码
            const char *start = ptr + 2;
            int i = 0;
            while (*start != ';' && *start != '\0')
            {
                password[i++] = *start++;
            }
            password[i] = '\0';

            // 显示密码长度（不显示明文）
            log_i("NFC", "密码长度: %d 字符", i);

            ptr = start;
        }
        else
        {
            ptr++;
        }
    }

    log_i("NFC", "WIFI配置解析完成:");
    log_i("NFC", "   WIFI名称: %s", ssid);
    log_i("NFC", "   加密方式: %s", auth_type);

    // 将配置写入KV存储
    if (strlen(ssid) > 0)
    {
        kv_set_string("wifi_ssid", ssid);
        log_i("NFC", "✅ SSID已保存到KV存储");

        if (strlen(password) > 0)
        {
            kv_set_string("wifi_password", password);
            log_i("NFC", "✅ 密码已保存到KV存储");
        }
        else
        {
            kv_set_string("wifi_password", "");
            log_i("NFC", "⚠ 开放网络，无需密码");
        }

        // 清除NDEF数据，确保下一次读取是空
        log_i("NFC", "正在清除NDEF数据...");
        if (nfc_clear_ndef_data())
        {
            log_i("NFC", "✅ NDEF数据清除成功，确保下次读取为空");
        }
        else
        {
            log_e("NFC", "❌ NDEF数据清除失败");
        }

        // LED快速闪烁5次表示检测到WIFI配置
        for (int j = 0; j < 5; j++)
        {
            LED_ON();
            Time_DelayMs(50);
            LED_OFF();
            Time_DelayMs(50);
        }

        log_i("NFC", "🎉 WIFI配置已保存，NDEF数据已清除，可重新启动网络连接");
    }
    else
    {
        log_e("NFC", "❌ SSID为空，无法保存配置");
    }
}

// 改进的NFC检测函数 - 基于template.c的实现
static bool check_nfc_tag(void)
{
    if (!g_nfc_initialized)
    {
        log_e("NFC", "❌ NFC模块未初始化，无法检测标签");
        return false;
    }

    log_i("NFC", "开始检测NFC标签...");

    int max_attempts = 3;
    for (int attempt = 1; attempt <= max_attempts; attempt++)
    {
        log_i("NFC", "尝试 %d/%d，请将NFC标签靠近开发板...", attempt, max_attempts);

        uint8_t ndefLen = 0;
        uint8_t ndef_Header = 0;
        uint32_t result_code = 0;

        // 尝试读取NDEF头信息
        result_code = NT3HReadHeaderNfc(&ndefLen, &ndef_Header);

        if (result_code == true)
        {
            log_i("NFC", "✅ NFC标签检测成功！");
            log_i("NFC", "   NDEF数据长度: %d 字节", ndefLen);

            // 尝试读取NDEF数据
            if (ndefLen > 0)
            {
                uint32_t total_len = ndefLen + NDEF_HEADER_SIZE; // 加上头大小
                log_i("NFC", "读取NDEF数据，总长度: %d 字节", total_len);

                uint8_t *ndefBuff = (uint8_t *)malloc(total_len + 1);
                if (ndefBuff != NULL)
                {
                    result_code = get_NDEFDataPackage(ndefBuff, total_len);
                    if (result_code == HI_ERR_SUCCESS)
                    {
                        log_i("NFC", "✅ NDEF数据读取成功");

                        // 显示原始数据用于调试
                        log_i("NFC", "原始NDEF数据预览:");
                        char debug_str[51] = {0};
                        int debug_idx = 0;
                        for (uint32_t i = 0; i < total_len && i < 50; i++)
                        {
                            if (ndefBuff[i] >= 32 && ndefBuff[i] <= 126)
                            {
                                debug_str[debug_idx++] = ndefBuff[i];
                            }
                            else
                            {
                                debug_str[debug_idx++] = '.';
                            }
                        }
                        debug_str[debug_idx] = '\0';
                        log_i("NFC", "   内容: %s", debug_str);

                        // 查找WIFI:前缀
                        int wifi_found = 0;
                        for (uint32_t i = 0; i < total_len - 5 && !wifi_found; i++)
                        {
                            if (memcmp(&ndefBuff[i], "WIFI:", 5) == 0)
                            {
                                wifi_found = 1;

                                // 提取整个WIFI配置字符串
                                char wifi_config[256] = {0};
                                int j = 0;

                                // 从WIFI:开始复制
                                for (uint32_t k = i; k < total_len && j < 255; k++)
                                {
                                    wifi_config[j++] = ndefBuff[k];

                                    // 检查是否到达配置结束（两个分号）
                                    if (k > i + 10 && ndefBuff[k] == ';' && k + 1 < total_len && ndefBuff[k + 1] == ';')
                                    {
                                        wifi_config[j++] = ';'; // 添加第二个分号
                                        wifi_config[j] = '\0';  // 结束字符串
                                        break;
                                    }
                                }

                                if (j > 0)
                                {
                                    wifi_config[j] = '\0';
                                    log_i("NFC", "🎉 检测到WIFI配置: %s", wifi_config);

                                    // 解析并保存WIFI配置
                                    parse_wifi_config(wifi_config);
                                }
                                else
                                {
                                    log_e("NFC", "❌ 无法提取WIFI配置字符串");
                                }

                                break;
                            }
                        }

                        if (!wifi_found)
                        {
                            log_w("NFC", "⚠ 未检测到WIFI配置");

                            // 显示更多调试信息
                            log_i("NFC", "原始数据十六进制:");
                            for (uint32_t i = 0; i < (total_len < 32 ? total_len : 32); i++)
                            {
                                if (i % 16 == 0)
                                    log_i("NFC", "  ");
                                log_i("NFC", "%02X ", ndefBuff[i]);
                            }
                            log_i("NFC", "");
                        }
                    }
                    else
                    {
                        log_e("NFC", "❌ NDEF数据读取失败，错误码: 0x%08X", result_code);
                    }

                    free(ndefBuff);
                }
                else
                {
                    log_e("NFC", "❌ 内存分配失败");
                }
            }
            else
            {
                log_w("NFC", "⚠ 标签中没有NDEF数据或数据长度为0");
            }

            log_i("NFC", "✅ 读取成功，等待下一个标签...");

            // LED长亮1秒表示成功
            LED_ON();
            Time_DelayMs(1000);
            LED_OFF();

            return true; // 检测成功，退出
        }
        else
        {
            log_i("NFC", "❌ 第 %d 次尝试失败", attempt);

            // 等待后重试
            if (attempt < max_attempts)
            {
                log_i("NFC", "等待2秒后重试...");
                Time_DelayMs(2000);
            }
        }
    }

    log_e("NFC", "❌ NFC标签检测失败");
    log_e("NFC", "可能原因:");
    log_e("NFC", "1. 没有放置NFC标签");
    log_e("NFC", "2. 标签距离过远");
    log_e("NFC", "3. 标签是空的");

    return false; // 检测失败
}

void Main_Task(void *arg)
{
    (void)arg;
    uint32_t current_time_ms = 0;

    // 初始化时间戳
    uint32_t g_last_sensor_update = Time_GetCurrentMs();
    uint32_t g_last_smoke_update = Time_GetCurrentMs();
    uint32_t g_last_network_status = Time_GetCurrentMs();
    uint32_t g_last_nfc_check = Time_GetCurrentMs();

    log_i("MAIN", "主任务开始运行");

    // 主循环
    while (1)
    {
        current_time_ms = Time_GetCurrentMs();

        // 1. 传感器读取（2秒间隔）
        if (current_time_ms - g_last_sensor_update >= SENSOR_UPDATE_INTERVAL_MS)
        {
            g_last_sensor_update = current_time_ms;

            // DHT11温湿度传感器读取
            if (dht11_read_data(&g_temperature, &g_humidity) == 0)
            {
                log_i("DHT11", "温湿度传感器: 湿度=%d%%, 温度=%d°C", g_humidity, g_temperature);
            }
            else
            {
                log_e("DHT11", "传感器读取失败");
            }
        }

        // 2. 烟雾传感器读取和报警（5秒间隔）
        if (current_time_ms - g_last_smoke_update >= SMOKE_UPDATE_INTERVAL_MS)
        {
            g_last_smoke_update = current_time_ms;

            // 读取烟雾传感器数据
            smoke_sensor_read_data(&g_smoke_data);

            // 记录烟雾传感器状态日志
            log_i("SMOKE", "烟雾传感器: ADC值=%d, 电压=%.2fV, 等级=%s",
                  g_smoke_data.raw_value, g_smoke_data.voltage,
                  smoke_sensor_get_level_string(g_smoke_data.level));

            // 检查烟雾报警
            if (g_smoke_data.alarm_triggered && !g_smoke_alarm_triggered)
            {
                // 首次触发报警
                g_smoke_alarm_triggered = true;
                log_e("SMOKE_ALARM", "⚠️ 烟雾报警触发！等级：%s",
                      smoke_sensor_get_level_string(g_smoke_data.level));

                // 触发蜂鸣器报警
                Buzzer_Alarm(5, 200, 100);

                // 闪烁LED报警
                for (int i = 0; i < 3; i++)
                {
                    LED_ON();
                    Time_DelayMs(200);
                    LED_OFF();
                    Time_DelayMs(200);
                }
            }
            else if (!g_smoke_data.alarm_triggered && g_smoke_alarm_triggered)
            {
                // 报警解除
                g_smoke_alarm_triggered = false;
                log_i("SMOKE_ALARM", "✅ 烟雾报警解除");
            }
            else if (g_smoke_data.alarm_triggered)
            {
                // 持续报警状态
                log_w("SMOKE_ALARM", "🚨 烟雾报警持续中，等级：%s",
                      smoke_sensor_get_level_string(g_smoke_data.level));
            }
        }

        // 3. NFC标签检测（5秒间隔）
        if (current_time_ms - g_last_nfc_check >= NFC_CHECK_INTERVAL_MS)
        {
            g_last_nfc_check = current_time_ms;

            if (g_nfc_initialized)
            {
                check_nfc_tag();
            }
            else
            {
                log_w("NFC", "NFC模块未初始化，跳过检测");
            }
        }

        // 4. 网络状态显示（30秒间隔）
        if (current_time_ms - g_last_network_status >= 30000)
        {
            g_last_network_status = current_time_ms;
            if (g_wifi_connected)
            {
                log_i("NETWORK", "WiFi状态: 已连接 %s, IP: %s", g_wifi_ssid, g_wifi_ip);
            }
            else
            {
                log_i("NETWORK", "网络状态: AP模式运行中, IP: %s", g_wifi_ip);
            }
        }

        // 添加短延时，让出CPU时间
        Time_DelayMs(10);
    }
}

static void Main_Entry(void)
{
    log_i("SYSTEM", "系统初始化开始");
    // 初始化库
    Time_Init();
    led_init(); // LED初始化（初始状态为关闭）
    Buzzer_Init();
    smoke_sensor_init();
    log_i("SMOKE", "烟雾传感器初始化完成");

    // NFC初始化 - 改进初始化逻辑，基于template.c
    log_i("NFC", "正在初始化NFC模块...");

    uint32_t nfc_ret = nfc_init();
    Time_DelayMs(500);
    if (nfc_ret == HI_ERR_SUCCESS)
    {
        g_nfc_initialized = true;
        log_i("NFC", "✅ NFC模块初始化成功");
        log_i("NFC", "请将包含WIFI配置的NFC标签靠近开发板");
        log_i("NFC", "支持的格式: WIFI:S:SSID;T:TYPE;P:PASSWORD;;");
        log_i("NFC", "NFC检测将每5秒执行一次");

        // 立即进行一次NFC检测
        log_i("NFC", "执行首次NFC检测...");
        if (check_nfc_tag())
        {
            log_i("NFC", "首次NFC检测完成");
        }
        else
        {
            log_i("NFC", "首次NFC检测未发现标签");
        }
    }
    else
    {
        log_e("NFC", "❌ NFC模块初始化失败，错误码: 0x%08X", nfc_ret);
        log_e("NFC", "请检查NFC模块连接和配置");
        g_nfc_initialized = false;
    }

    // 初始化按键管理器
    log_i("KEY", "正在初始化按键管理器...");
    g_key_manager = key_manager_create(2); // 最多支持2个按键
    if (g_key_manager != NULL)
    {
        // 添加按键1 (GPIO11)
        int8_t key1_index = key_manager_add_key(g_key_manager, KEY1_PIN, 0, 10, 1000, 300);
        if (key1_index >= 0)
        {
            log_i("KEY", "按键1 (GPIO13) 添加成功，索引: %d", key1_index);
        }
        else
        {
            log_e("KEY", "按键1添加失败");
        }

        // 添加按键2 (GPIO12)
        int8_t key2_index = key_manager_add_key(g_key_manager, KEY2_PIN, 0, 10, 1000, 300);
        if (key2_index >= 0)
        {
            log_i("KEY", "按键2 (GPIO12) 添加成功，索引: %d", key2_index);
        }
        else
        {
            log_e("KEY", "按键2添加失败");
        }

        // 创建按键定时器（5ms周期）
        g_key_timer = osTimerNew(KeyTimerCb, osTimerPeriodic, NULL, NULL);
        if (g_key_timer != NULL)
        {
            osTimerStart(g_key_timer, 5);
            log_i("KEY", "按键定时器启动成功 (5ms周期)");
        }
        else
        {
            log_e("KEY", "按键定时器创建失败");
        }
    }
    else
    {
        log_e("KEY", "按键管理器创建失败");
    }

    // 初始化KV存储模块
    kv_result_t kv_ret = kv_init();
    if (kv_ret == KV_SUCCESS)
    {
        log_i("KV", "KV存储模块初始化成功");
        // KV断电保存验证测试
        log_i("KV_TEST", "开始KV断电保存验证测试...");

        // 检查是否是第一次运行
        char boot_count_str[16] = {0};
        kv_result_t boot_count_result = kv_get_string("boot_count", boot_count_str, sizeof(boot_count_str));

        if (boot_count_result == KV_ERROR_KEY_NOT_FOUND)
        {
            // 第一次运行：设置初始数据
            log_i("KV_TEST", "检测到第一次运行，设置初始KV数据...");

            // 设置启动计数为1
            kv_set_string("boot_count", "1");

            // 设置测试字符串
            const char *test_string = "Hi3861_KV_Test_FirstBoot";
            kv_set_string("test_key", test_string);

            // 设置时间戳
            uint32_t current_time = Time_GetCurrentMs();
            kv_set_uint32("first_boot_time", current_time);

            // 设置设备信息
            const char *device_info = "Hi3861_WiFi_IoT_Device_v1.0_FirstBoot";
            kv_set_string("device_info", device_info);

            log_i("KV_TEST", "✅ 第一次运行：KV数据初始化完成");
            log_i("KV_TEST", "   测试字符串: %s", test_string);
            log_i("KV_TEST", "   启动时间: %u ms", current_time);
            log_i("KV_TEST", "   设备信息: %s", device_info);
            log_i("KV_TEST", "💡 请断电重启设备，验证KV数据是否保存");
        }
        else if (boot_count_result == KV_SUCCESS)
        {
            // 后续运行：验证数据是否保存
            log_i("KV_TEST", "检测到后续运行，验证KV数据保存...");

            // 读取并更新启动计数
            int boot_count = atoi(boot_count_str);
            boot_count++;
            char new_boot_count[16];
            snprintf(new_boot_count, sizeof(new_boot_count), "%d", boot_count);
            kv_set_string("boot_count", new_boot_count);

            // 验证测试字符串
            char test_buffer[64] = {0};
            kv_result_t test_result = kv_get_string("test_key", test_buffer, sizeof(test_buffer));
            if (test_result == KV_SUCCESS)
            {
                log_i("KV_TEST", "✅ 测试字符串验证成功: %s", test_buffer);
            }
            else
            {
                log_e("KV_TEST", "❌ 测试字符串验证失败，错误码: %d", test_result);
            } // 添加缺失的右大括号
            // 验证启动时间
            uint32_t first_boot_time = 0;
            kv_result_t time_result = kv_get_uint32("first_boot_time", &first_boot_time);
            if (time_result == KV_SUCCESS)
            {
                log_i("KV_TEST", "✅ 首次启动时间验证成功: %u ms", first_boot_time);
            }
            else
            {
                log_e("KV_TEST", "❌ 首次启动时间验证失败，错误码: %d", time_result);
            }

            // 验证设备信息
            char device_buffer[64] = {0};
            kv_result_t device_result = kv_get_string("device_info", device_buffer, sizeof(device_buffer));
            if (device_result == KV_SUCCESS)
            {
                log_i("KV_TEST", "✅ 设备信息验证成功: %s", device_buffer);
            }
            else
            {
                log_e("KV_TEST", "❌ 设备信息验证失败，错误码: %d", device_result);
            }

            // 显示启动次数
            log_i("KV_TEST", "📊 当前启动次数: %d", boot_count);
            // 验证中文支持
            char chinese_buffer[64] = {0};
            kv_result_t chinese_result = kv_get_string("chinese_test", chinese_buffer, sizeof(chinese_buffer));
            if (chinese_result == KV_SUCCESS)
            {
                log_i("KV_TEST", "✅ 中文字符串验证成功: %s", chinese_buffer);
            }
            else if (chinese_result == KV_ERROR_KEY_NOT_FOUND)
            {
                // 如果是第一次看到中文测试失败，可能是之前没有设置，现在设置一下
                const char *chinese_test = "KV存储测试-中文验证";

                kv_set_string("chinese_test", chinese_test);
                log_i("KV_TEST", "💡 设置中文字符串: %s", chinese_test);
            }
            else
            {
                log_e("KV_TEST", "❌ 中文字符串验证失败，错误码: %d", chinese_result);
            }

            log_i("KV_TEST", "🎉 KV断电保存验证完成！");
        }
        else
        {
            log_e("KV_TEST", "❌ 启动计数读取失败，错误码: %d", boot_count_result);
        }

        // 初始化默认配置（如果不存在）
        uint32_t scan_timeout, connect_timeout;
        if (kv_get_uint32("scan_timeout", &scan_timeout) != KV_SUCCESS)
        {
            kv_set_uint32("scan_timeout", 30000);
            log_i("KV", "设置默认扫描超时: 30秒");
        }
        if (kv_get_uint32("connect_timeout", &connect_timeout) != KV_SUCCESS)
        {
            kv_set_uint32("connect_timeout", 60000);
            log_i("KV", "设置默认连接超时: 60秒");
        }
    }
    else
    {
        log_w("KV", "KV存储模块初始化失败，错误码: %d", kv_ret);
    }

    // 初始化DHT11
    log_i("DHT11", "正在初始化DHT11传感器...");
    uint8_t retry_count = 0;
    while (dht11_init())
    {
        log_w("DHT11", "初始化失败，正在重试... (第%d次/共%d次)", retry_count + 1, DHT11_RETRY_COUNT);
        retry_count++;
        Time_DelayMsPrecise(1000);
        if (retry_count >= DHT11_RETRY_COUNT)
        {
            log_e("DHT11", "初始化失败，已达到最大重试次数: %d", DHT11_RETRY_COUNT);
            g_dht11_connected = false;
            break;
        }
    }

    if (g_dht11_connected)
    {
        log_i("DHT11", "DHT11传感器初始化成功");
    }

    // 创建蜂鸣器tick定时器
    log_i("BUZZER", "正在创建蜂鸣器定时器...");
    g_buzzer_tick_timer = osTimerNew(BuzzerTickCb, osTimerPeriodic, NULL, NULL);
    if (g_buzzer_tick_timer != NULL)
    {
        osTimerStart(g_buzzer_tick_timer, 5);
        log_i("BUZZER", "蜂鸣器定时器启动成功");
    }
    else
    {
        log_e("BUZZER", "蜂鸣器定时器创建失败");
    }

    // 启动提示音
    Buzzer_Alarm(2, 50, 100);

    log_i("SYSTEM", "系统初始化完成");
    // 创建主任务
    log_i("SYSTEM", "正在创建主任务...");
    osThreadAttr_t main_task_attr = {
        .name = "MainTask",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U,
        .stack_mem = NULL,
        .stack_size = 8192,
        .priority = osPriorityNormal};

    if (osThreadNew(Main_Task, NULL, &main_task_attr) == NULL)
    {
        log_e("SYSTEM", "主任务创建失败!");
    }
    else
    {
        log_i("SYSTEM", "主任务创建成功");
    }

    // 创建网络任务（合并PHP API功能）
    log_i("SYSTEM", "正在创建网络任务...");
    osThreadAttr_t network_attr = {
        .name = "NetworkTask",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U,
        .stack_mem = NULL,
        .stack_size = 8192, // 增加栈大小以容纳合并后的功能
        .priority = osPriorityAboveNormal};

    if (osThreadNew(network_task, NULL, &network_attr) == NULL)
    {
        log_e("NETWORK", "网络任务创建失败!");
    }
    else
    {
        log_i("NETWORK", "网络任务创建成功");
    }
}

// 应用入口函数
SYS_RUN(Main_Entry);