#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "hi_io.h"
#include "hi_gpio.h"
#include "buzzer.h"
#include "dht11.h"
#include "time.h"    // 使用新的时间库
#include "wifi.h"  // 修改为相对路径
#include "kv.h"     // 新增KV存储模块头文件
#include "php_api.h" // 恢复PHP API模块头文件

//#define DEFAULT_DEBUG_LEVEL 0
#include "debug.h"   // 新增调试头文件

// 函数原型声明
void led_init(void);
void network_task(void *arg); // 新增网络任务声明
void php_api_task(void *arg); // 恢复PHP API任务声明

// 管脚定义
#define LED_PIN         HI_IO_NAME_GPIO_2
#define LED_GPIO_FUN    HI_IO_FUNC_GPIO_2_GPIO

// LED控制宏定义（修复函数名错误）
#define LED_ON()        hi_gpio_set_ouput_val(LED_PIN, HI_GPIO_VALUE0)
#define LED_OFF()       hi_gpio_set_ouput_val(LED_PIN, HI_GPIO_VALUE1)

static osTimerId_t g_buzzer_tick_timer = NULL;

// 全局变量用于存储传感器数据
uint8_t g_temperature = 255;  // 改为uint8_t类型
uint8_t g_humidity = 255;     // 改为uint8_t类型
static bool g_dht11_connected = true;

// 网络状态全局变量
static bool g_wifi_connected = false;
static char g_wifi_ssid[32] = "";
static char g_wifi_ip[16] = "";

static uint8_t led_value = 0;

// 常量定义（毫秒）
#define SENSOR_UPDATE_INTERVAL_MS 2000    // 2秒读取一次
#define LED_UPDATE_INTERVAL_MS    100     // 100ms LED闪烁
#define NETWORK_CHECK_INTERVAL_MS 10000   // 10秒检查一次网络状态

// DHT11读取重试机制
#define DHT11_RETRY_COUNT 3            // 每次读取最多重试3次

static void BuzzerTickCb(void *arg) {
    (void)arg;
    Buzzer_Tick(Time_GetCurrentMs());
}

// 网络任务函数 - 使用KV存储系统
void network_task(void *arg) {
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
        kv_get_string("wifi_password", wifi_password, sizeof(wifi_password)) == KV_SUCCESS) {
        
        log_i("NETWORK", "从KV存储读取到WiFi配置: %s", wifi_ssid);
        
        // 尝试连接STA网络
        log_i("NETWORK", "正在连接到WiFi网络: %s", wifi_ssid);
        WifiErrorCode wifi_ret = WiFi_connectHotspots(wifi_ssid, wifi_password);
        
        if (wifi_ret == WIFI_SUCCESS) {
            log_i("NETWORK", "WiFi连接成功");
            g_wifi_connected = true;
            strncpy(g_wifi_ssid, wifi_ssid, sizeof(g_wifi_ssid) - 1);
            
            // 获取IP地址
            char *ip_addr = WiFi_GetLocalIP();
            if (ip_addr != NULL) {
                strncpy(g_wifi_ip, ip_addr, sizeof(g_wifi_ip) - 1);
                log_i("NETWORK", "获取到IP地址: %s", g_wifi_ip);
            }
        } else {
            log_w("NETWORK", "WiFi连接失败，错误码: %d", wifi_ret);
        }
    } else {
        log_i("NETWORK", "KV存储中没有找到WiFi配置");
    }
    
    // 如果STA连接失败或没有配置，创建AP模式
    if (!g_wifi_connected) {
        log_i("NETWORK", "启动AP模式作为备用方案");
        const char *ap_ssid = "Hi3861_Config_AP";
        const char *ap_password = "12345678";
        
        WifiErrorCode ap_ret = WiFi_createHotspots(ap_ssid, ap_password);
        if (ap_ret == WIFI_SUCCESS) {
            log_i("NETWORK", "AP热点创建成功: %s", ap_ssid);
            
            // 获取AP的IP地址
            char *ip_addr = WiFi_GetLocalIP();
            if (ip_addr != NULL) {
                strncpy(g_wifi_ip, ip_addr, sizeof(g_wifi_ip) - 1);
                log_i("NETWORK", "AP热点IP地址: %s", g_wifi_ip);
            }
        } else {
            log_e("NETWORK", "AP热点创建失败，错误码: %d", ap_ret);
        }
    }
    
    // 启动HTTP服务器
    log_i("NETWORK", "正在启动HTTP服务器...");
    php_api_result_t php_ret = php_api_start_server();
    if (php_ret == PHP_API_SUCCESS) {
        log_i("NETWORK", "HTTP服务器启动成功");
    } else {
        log_w("NETWORK", "HTTP服务器启动失败，错误码: %d", php_ret);
    }
    
    log_i("NETWORK", "网络任务初始化完成，进入主循环");
    
    // 网络任务主循环
    while (1) {
        sleep(10);
        
        if (g_wifi_connected) {
            log_i("NETWORK", "WiFi状态: 已连接 %s, IP: %s", g_wifi_ssid, g_wifi_ip);
        } else {
            log_i("NETWORK", "网络状态: AP模式运行中, IP: %s", g_wifi_ip);
        }
    }
}

// PHP API任务函数
void php_api_task(void *arg) {
    (void)arg;
    
    log_i("PHP_API", "PHP API任务开始启动");
    
    // 等待网络初始化完成
    sleep(5);
    
    // 初始化PHP API模块
    log_i("PHP_API", "正在初始化PHP API模块...");
    php_api_result_t php_ret = php_api_init();
    if (php_ret != PHP_API_SUCCESS) {
        log_e("PHP_API", "PHP API初始化失败，错误码: %d", php_ret);
        return;
    }
    
    log_i("PHP_API", "PHP API初始化成功");
    
    // API任务主循环
    log_i("PHP_API", "PHP API任务进入主循环");
    while (1) {
        sleep(1);
    }
}

// 主任务
static void Main_Task(void) {
    uint32_t current_time_ms;
    
    // 初始化时间戳
    uint32_t g_last_sensor_update = Time_GetCurrentMs();
    uint32_t g_last_led_update = Time_GetCurrentMs();
    uint32_t g_last_network_status = Time_GetCurrentMs();

    // 设置终端字符编码
    printf("\033%%G");
    
    log_i("MAIN", "主任务开始运行");
    
    while (1) {
        current_time_ms = Time_GetCurrentMs();
        
        // 1. 传感器读取（2秒间隔）
        if (current_time_ms - g_last_sensor_update >= SENSOR_UPDATE_INTERVAL_MS) {
            g_last_sensor_update = current_time_ms;

            if (dht11_read_data(&g_temperature, &g_humidity) == 0) {
                log_i("DHT11", "温湿度传感器: 湿度=%d%%, 温度=%d°C", g_humidity, g_temperature);
            } else {
                log_e("DHT11", "传感器读取失败");
            }
        }
        
        // 2. LED显示更新（100ms间隔）
        if (current_time_ms - g_last_led_update >= LED_UPDATE_INTERVAL_MS) {
            g_last_led_update = current_time_ms;
            
            // LED闪烁控制
            if (led_value) {
                LED_ON();
            } else {
                LED_OFF();
            }
            led_value = !led_value;
        }
        
        // 3. 定期显示网络状态（30秒间隔）
        if (current_time_ms - g_last_network_status >= 30000) {
            g_last_network_status = current_time_ms;
            
            if (g_wifi_connected) {
                log_i("STATUS", "系统状态: WiFi已连接 %s, IP: %s", g_wifi_ssid, g_wifi_ip);
            } else {
                log_i("STATUS", "系统状态: AP模式运行中, IP: %s", g_wifi_ip);
            }
            
            // 显示传感器状态
            if (g_dht11_connected) {
                log_i("STATUS", "传感器状态: 温度=%d°C, 湿度=%d%%", g_temperature, g_humidity);
            } else {
                log_w("STATUS", "传感器状态: DHT11未连接");
            }
        }
        
        // 添加延时，让出CPU时间
        Time_DelayMs(10);
    }
}

static void Main_Entry(void) {
    // 初始化库
    Time_Init();
    led_init();
    Buzzer_Init();

    log_i("SYSTEM", "系统初始化开始");

    // 初始化KV存储模块
    kv_result_t kv_ret = kv_init();
    if (kv_ret == KV_SUCCESS) {
        log_i("KV", "KV存储模块初始化成功");
        
        // KV断电保存验证测试
        log_i("KV_TEST", "开始KV断电保存验证测试...");
        
        // 检查是否是第一次运行
        char boot_count_str[16] = {0};
        kv_result_t boot_count_result = kv_get_string("boot_count", boot_count_str, sizeof(boot_count_str));
        
        if (boot_count_result == KV_ERROR_KEY_NOT_FOUND) {
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
            
        } else if (boot_count_result == KV_SUCCESS) {
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
            if (test_result == KV_SUCCESS) {
                log_i("KV_TEST", "✅ 测试字符串验证成功: %s", test_buffer);
            } else {
                log_e("KV_TEST", "❌ 测试字符串验证失败，错误码: %d", test_result);
            }
            
            // 验证启动时间
            uint32_t first_boot_time = 0;
            kv_result_t time_result = kv_get_uint32("first_boot_time", &first_boot_time);
            if (time_result == KV_SUCCESS) {
                log_i("KV_TEST", "✅ 首次启动时间验证成功: %u ms", first_boot_time);
            } else {
                log_e("KV_TEST", "❌ 首次启动时间验证失败，错误码: %d", time_result);
            }
            
            // 验证设备信息
            char device_buffer[64] = {0};
            kv_result_t device_result = kv_get_string("device_info", device_buffer, sizeof(device_buffer));
            if (device_result == KV_SUCCESS) {
                log_i("KV_TEST", "✅ 设备信息验证成功: %s", device_buffer);
            } else {
                log_e("KV_TEST", "❌ 设备信息验证失败，错误码: %d", device_result);
            }
            
            // 显示启动次数
            log_i("KV_TEST", "📊 当前启动次数: %d", boot_count);
            
            // 验证中文支持
            char chinese_buffer[64] = {0};
            kv_result_t chinese_result = kv_get_string("chinese_test", chinese_buffer, sizeof(chinese_buffer));
            if (chinese_result == KV_SUCCESS) {
                log_i("KV_TEST", "✅ 中文字符串验证成功: %s", chinese_buffer);
            } else if (chinese_result == KV_ERROR_KEY_NOT_FOUND) {
                // 如果是第一次看到中文测试失败，可能是之前没有设置，现在设置一下
                const char *chinese_test = "KV存储测试-中文验证";
                kv_set_string("chinese_test", chinese_test);
                log_i("KV_TEST", "💡 设置中文字符串: %s", chinese_test);
            } else {
                log_e("KV_TEST", "❌ 中文字符串验证失败，错误码: %d", chinese_result);
            }
            
            log_i("KV_TEST", "🎉 KV断电保存验证完成！");
            
        } else {
            log_e("KV_TEST", "❌ 启动计数读取失败，错误码: %d", boot_count_result);
        }
        
        // 初始化默认配置（如果不存在）
        uint32_t scan_timeout, connect_timeout;
        if (kv_get_uint32("scan_timeout", &scan_timeout) != KV_SUCCESS) {
            kv_set_uint32("scan_timeout", 30000);
            log_i("KV", "设置默认扫描超时: 30秒");
        }
        if (kv_get_uint32("connect_timeout", &connect_timeout) != KV_SUCCESS) {
            kv_set_uint32("connect_timeout", 60000);
            log_i("KV", "设置默认连接超时: 60秒");
        }
        
    } else {
        log_w("KV", "KV存储模块初始化失败，错误码: %d", kv_ret);
    }

    // 初始化DHT11
    log_i("DHT11", "正在初始化DHT11传感器...");
    uint8_t retry_count = 0;
    while (dht11_init()) {
        log_w("DHT11", "初始化失败，正在重试... (第%d次/共%d次)", retry_count + 1, DHT11_RETRY_COUNT);
        retry_count++;
        Time_DelayMsPrecise(1000);
        if (retry_count >= DHT11_RETRY_COUNT) {
            log_e("DHT11", "初始化失败，已达到最大重试次数: %d", DHT11_RETRY_COUNT);
            g_dht11_connected = false;
            break;
        }
    }
    
    if (g_dht11_connected) {
        log_i("DHT11", "DHT11传感器初始化成功");
    }

    // 创建蜂鸣器tick定时器
    log_i("BUZZER", "正在创建蜂鸣器定时器...");
    g_buzzer_tick_timer = osTimerNew(BuzzerTickCb, osTimerPeriodic, NULL, NULL);
    if (g_buzzer_tick_timer != NULL) {
        osTimerStart(g_buzzer_tick_timer, 5);
        log_i("BUZZER", "蜂鸣器定时器启动成功");
    } else {
        log_e("BUZZER", "蜂鸣器定时器创建失败");
    }

    // 启动提示音
    Buzzer_Alarm(2, 50, 100);
    log_i("BUZZER", "启动提示音已触发");
    
    // 创建网络任务
    log_i("SYSTEM", "正在创建网络任务...");
    osThreadAttr_t network_attr = {
        .name = "NetworkTask",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U,
        .stack_mem = NULL,
        .stack_size = 8192,
        .priority = osPriorityAboveNormal
    };
    
    if (osThreadNew(network_task, NULL, &network_attr) == NULL) {
        log_e("NETWORK", "网络任务创建失败!");
    } else {
        log_i("NETWORK", "网络任务创建成功");
    }
    
    // 创建PHP API任务
    log_i("SYSTEM", "正在创建PHP API任务...");
    osThreadAttr_t php_api_attr = {
        .name = "PHP_API_Task",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U,
        .stack_mem = NULL,
        .stack_size = 8192,
        .priority = osPriorityNormal
    };
    
    if (osThreadNew(php_api_task, NULL, &php_api_attr) == NULL) {
        log_e("PHP_API", "PHP API任务创建失败!");
    } else {
        log_i("PHP_API", "PHP API任务创建成功");
    }
    
    log_i("SYSTEM", "系统初始化完成");
    
    // 创建主任务
    log_i("SYSTEM", "正在创建主任务...");
    osThreadAttr_t main_task_attr = {
        .name = "MainTask",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U,
        .stack_mem = NULL,
        .stack_size = 4096,
        .priority = osPriorityNormal
    };
    
    if (osThreadNew(Main_Task, NULL, &main_task_attr) == NULL) {
        log_e("SYSTEM", "主任务创建失败!");
    } else {
        log_i("SYSTEM", "主任务创建成功");
    }
}

// 应用入口函数
SYS_RUN(Main_Entry);

void led_init(void) {
    hi_gpio_init();                                            // GPIO初始化
    hi_io_set_pull(LED_PIN, HI_IO_PULL_DOWN);                  // 设置GPIO下拉
    hi_io_set_func(LED_PIN, LED_GPIO_FUN);                     // 设置IO为GPIO功能
    hi_gpio_set_dir(LED_PIN, HI_GPIO_DIR_OUT);                 // 设置GPIO为输出模式
    log_i("LED", "LED初始化完成");
}