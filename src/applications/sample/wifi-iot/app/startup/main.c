#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "NT3H.h"
#include "alarm_system.h"
#include "buzzer.h"
#include "cmsis_os2.h"
#include "dht11.h"
#include "hi_gpio.h"
#include "hi_io.h"
#include "hi_time.h"
#include "hi_timer.h"
#include "key_manager.h"  // 新增按键管理器头文件
#include "kv.h"           // 新增KV存储模块头文件
#include "lightsense.h"
#include "nfc.h"
#include "ohos_init.h"
#include "oled.h"
#include "oled_driver.h"
#include "bitmap_logo.h"   // 开机 Logo
#include "php_api.h"  // 恢复PHP API模块头文件
#include "smoke.h"
#include "time.h"  // 使用新的时间库
#include "wifi.h"  // 修改为相对路径

#define DEFAULT_DEBUG_LEVEL DEBUG_LEVEL_INFO
#include "debug.h"  // 新增调试头文件

// http://192.168.0.1

/*
#        ┏┓　　　┏┓+ +
#　　 　┏┛┻━━━┛┻┓ + +
#　　 　┃　　　　　 ┃ 　
#　　 　┃　　　━　  ┃ ++ + + +
#　　  ████━████   ┃  +
#　　 　┃　　　　　 ┃ +
#　　 　┃　　　┻　　┃
#　　 　┃　　　　　 ┃ + +
#0　　 　┗━┓　　　┏━┛
#　　　　　┃　　　┃　　　　　　　　　　　
#　　　　　┃　　　┃ + + + +
#　　　　　┃　　　┃　　　　Codes are far away from bugs with the animal
protecting #　　　　　┃　　　┃ + 　　　　神兽保佑,代码无bug　　
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
void network_task(void* arg);  // 修改为合并后的网络任务声明
void alarm_event_handler(const alarm_event_t* event);

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
osTimerId_t g_key_timer = NULL;  // 新增按键定时器

// 全局按键管理器
key_manager_t* g_key_manager = NULL;

// MG995 舵机控制（GPIO5，标准50Hz软件PWM）
#define SERVO_PIN     HI_IO_NAME_GPIO_5
#define SERVO_GPIO_FUN HI_IO_FUNC_GPIO_5_GPIO
#define SERVO_ANGLE_MIN 0
#define SERVO_ANGLE_MAX 180
#define SERVO_PULSES 50       // 大扭矩舵机惯性大，多发几拍确保到位
#define SERVO_PERIOD_US 20000 // 50Hz = 20ms 周期
static uint16_t g_servo_angle = 90;  // 当前角度 0~180（默认中位90）

static void servo_init(void);
static void servo_set_angle(uint16_t angle);
static void servo_test(void);

uint8_t g_running_status = 0;

// 新增持续报警相关变量 - 修改为支持多报警类型
static bool g_alarm_active[3] = {false, false,
                                 false};  // 温度、湿度、烟雾报警状态
static alarm_level_t g_current_alarm_level[3] = {
    ALARM_LEVEL_NONE, ALARM_LEVEL_NONE,
    ALARM_LEVEL_NONE};                               // 各报警类型当前级别
static uint32_t g_last_alarm_action[3] = {0, 0, 0};  // 各报警类型最后动作时间
#define ALARM_ACTION_INTERVAL_MS 500                 // 报警动作间隔500ms

// 全局变量用于存储传感器数据
uint8_t g_temperature = 0;  // 改为uint8_t类型
uint8_t g_humidity = 0;     // 改为uint8_t类型
bool g_dht11_connected = true;

// 烟雾报警器相关变量 - 新增缺失的变量定义
bool g_smoke_alarm_triggered = false;
smoke_sensor_data_t g_smoke_data = {0};

// 烟雾传感器更新间隔常量
#define SMOKE_UPDATE_INTERVAL_MS 1 * 1000  // 1秒读取一次

uint8_t led_value = 0;

// 常量定义（毫秒）
#define SENSOR_UPDATE_INTERVAL_MS 2 * 1000   // 2秒读取一次
#define NETWORK_CHECK_INTERVAL_MS 10 * 1000  // 10秒检查一次网络状态
#define PHP_UPDATE_INTERVAL_MS 100

// NFC检测相关变量
bool g_nfc_initialized = false;
uint32_t g_last_nfc_check = 0;
#define NFC_CHECK_INTERVAL_MS 5 * 1000  // 15秒检测一次NFC标签

// DHT11读取重试机制
#define DHT11_RETRY_COUNT 3  // 每次读取最多重试3次

// 蜂鸣器定时器回调函数
void BuzzerTickCb(void* arg) {
  (void)arg;
  Buzzer_Tick(Time_GetCurrentMs());
}
void alarm_event_handler(const alarm_event_t* event) {
  if (event == NULL) return;

  log_i("ALARM_SYSTEM", "报警事件: 类型=%s, 级别=%s, 值=%u, 阈值=%u",
        alarm_system_get_type_string(event->type),
        alarm_system_get_level_string(event->level), event->value,
        event->threshold);

  // 根据报警类型更新对应的报警状态
  uint8_t alarm_index = (uint8_t)event->type;

  if (event->level == ALARM_LEVEL_NONE) {
    // 报警解除
    g_alarm_active[alarm_index] = false;
    g_current_alarm_level[alarm_index] = ALARM_LEVEL_NONE;
    log_i("ALARM_SYSTEM", "%s报警已解除",
          alarm_system_get_type_string(event->type));
  } else {
    // 报警触发
    g_alarm_active[alarm_index] = true;
    g_current_alarm_level[alarm_index] = event->level;
    g_last_alarm_action[alarm_index] = Time_GetCurrentMs();
    log_i("ALARM_SYSTEM", "%s报警已触发，级别：%s",
          alarm_system_get_type_string(event->type),
          alarm_system_get_level_string(event->level));

    // 立即执行相应的蜂鸣器报警
    switch (event->type) {
      case ALARM_TYPE_TEMPERATURE:
        if (event->level == ALARM_LEVEL_YELLOW) {
          Buzzer_Alarm(1, 250, 0);
        } else if (event->level == ALARM_LEVEL_RED) {
          Buzzer_Alarm(3, 100, 0);
        }
        break;

      case ALARM_TYPE_HUMIDITY:
        if (event->level == ALARM_LEVEL_YELLOW) {
          Buzzer_Alarm(1, 500, 0);
        } else if (event->level == ALARM_LEVEL_RED) {
          Buzzer_Alarm(3, 100, 0);
        }
        break;

      case ALARM_TYPE_SMOKE:
        if (event->level == ALARM_LEVEL_YELLOW) {
          Buzzer_Alarm(2, 50, 0);
        } else if (event->level == ALARM_LEVEL_RED) {
          Buzzer_Alarm(4, 50, 0);
        }
        break;

      default:
        break;
    }
  }

  // 更新全局运行状态（取最高报警级别）
  alarm_level_t highest_level = ALARM_LEVEL_NONE;
  bool any_alarm_active = false;

  for (int i = 0; i < 3; i++) {
    if (g_alarm_active[i]) {
      any_alarm_active = true;
      if (g_current_alarm_level[i] > highest_level) {
        highest_level = g_current_alarm_level[i];
      }
    }
  }

  if (!any_alarm_active) {
    g_running_status = 0;
    log_i("RUNNING_STATUS", "所有报警已解除");
  } else {
    g_running_status = (uint8_t)highest_level;
  }

  log_i("RUNNING_STATUS", "系统状态更新: g_running_status=%d, 活跃报警数=%d",
        g_running_status,
        (g_alarm_active[0] ? 1 : 0) + (g_alarm_active[1] ? 1 : 0) +
            (g_alarm_active[2] ? 1 : 0));
}

// 持续报警处理函数 - 修改为支持多报警类型独立处理
static void handle_continuous_alarm(void) {
  uint32_t current_time = Time_GetCurrentMs();
  bool any_alarm_active = false;
  static bool need_fan = false;          // 改为静态变量，保持状态
  static bool yellow_led_state = false;  // 黄色LED状态
  static bool red_led_state = false;     // 红色LED状态
  static bool last_fan_state = false;    // 记录上一次风扇状态
  static bool last_alarm_state = false;  // 记录上一次报警状态

  // 检查是否需要风扇的报警类型是否存在
  bool current_need_fan = false;
  for (int i = 0; i < 3; i++) {
    if (g_alarm_active[i]) {
      any_alarm_active = true;

      // 检查是否需要风扇的报警类型
      if ((i == ALARM_TYPE_TEMPERATURE &&
           g_current_alarm_level[i] == ALARM_LEVEL_RED) ||
          (i == ALARM_TYPE_SMOKE &&
           g_current_alarm_level[i] >= ALARM_LEVEL_YELLOW) ||
          (i == ALARM_TYPE_SMOKE &&
           g_current_alarm_level[i] >= ALARM_LEVEL_RED)) {
        current_need_fan = true;
      }

      // 检查是否到达该报警类型的动作间隔
      if (current_time - g_last_alarm_action[i] >= ALARM_ACTION_INTERVAL_MS) {
        g_last_alarm_action[i] = current_time;

        // 根据报警类型和级别执行不同的持续报警动作
        switch ((alarm_type_t)i) {
          case ALARM_TYPE_TEMPERATURE:
            // 温度报警处理
            switch (g_current_alarm_level[i]) {
              case ALARM_LEVEL_YELLOW:
                // 温度黄色报警：蜂鸣器单次报警
                Buzzer_Alarm(1, 250, 0);  // 1次，250ms，无间隔
                log_d("ALARM", "温度黄色报警持续动作");
                break;

              case ALARM_LEVEL_RED:
                // 温度红色报警：蜂鸣器3次报警
                Buzzer_Alarm(3, 100, 0);  // 3次，100ms，无间隔
                log_d("ALARM", "温度红色报警持续动作");
                break;

              default:
                break;
            }
            break;
          case ALARM_TYPE_HUMIDITY:
            // 湿度报警处理
            switch (g_current_alarm_level[i]) {
              case ALARM_LEVEL_YELLOW:
                // 湿度黄色报警：蜂鸣器单次报警
                Buzzer_Alarm(1, 500, 0);  // 1次，500ms，无间隔
                log_d("ALARM", "湿度黄色报警持续动作");
                break;

              case ALARM_LEVEL_RED:
                // 湿度红色报警：蜂鸣器3次报警
                Buzzer_Alarm(3, 100, 0);  // 3次，100ms，无间隔
                log_d("ALARM", "湿度红色报警持续动作");
                break;

              default:
                break;
            }
            break;

          case ALARM_TYPE_SMOKE:
            // 烟雾报警处理
            switch (g_current_alarm_level[i]) {
              case ALARM_LEVEL_YELLOW:
                // 烟雾黄色报警：蜂鸣器2次报警
                Buzzer_Alarm(2, 50, 0);  // 2次，50ms，无间隔
                log_d("ALARM", "烟雾黄色报警持续动作");
                break;

              case ALARM_LEVEL_RED:
                // 烟雾红色报警：蜂鸣器4次报警
                Buzzer_Alarm(4, 50, 0);  // 4次，50ms，无间隔
                log_d("ALARM", "烟雾红色报警持续动作");
                break;

              default:
                break;
            }
            break;

          default:
            break;
        }
      }
    }
  }

  // 更新need_fan状态（只在状态变化时更新）
  if (current_need_fan != need_fan) {
    need_fan = current_need_fan;
    log_i("FAN", "风扇需求状态变化: %s", need_fan ? "需要风扇" : "不需要风扇");
  }

  // LED控制逻辑：根据最高报警级别控制LED
  alarm_level_t highest_level = ALARM_LEVEL_NONE;
  for (int i = 0; i < 3; i++) {
    if (g_alarm_active[i] && g_current_alarm_level[i] > highest_level) {
      highest_level = g_current_alarm_level[i];
    }
  }

  // 根据最高报警级别控制LED
  switch (highest_level) {
    case ALARM_LEVEL_YELLOW:
      // 黄色报警：LED慢速闪烁
      if (yellow_led_state) {
        LED_ON();
        Time_DelayMs(300);
        LED_OFF();
        Time_DelayMs(400);
      } else {
        LED_OFF();
        Time_DelayMs(600);
      }
      yellow_led_state = !yellow_led_state;
      break;

    case ALARM_LEVEL_RED:
      // 红色报警：LED快速闪烁
      if (red_led_state) {
        LED_ON();
        Time_DelayMs(150);
        LED_OFF();
        Time_DelayMs(150);
      } else {
        LED_ON();
        Time_DelayMs(150);
        LED_OFF();
        Time_DelayMs(150);
      }
      red_led_state = !red_led_state;
      break;

    case ALARM_LEVEL_NONE:
    default:
      // 无报警：LED关闭
      LED_OFF();
      break;
  }

  // 风扇控制 [已注释：继电器 GPIO8 腾给步进电机]
  // bool current_fan_state = need_fan;
  // if (current_fan_state != last_fan_state) {
  //   if (current_fan_state) {
  //     relay_set_state(RELAY_ON);
  //     log_i("RELAY", "🔧 风扇已开启 - 温度红色或烟雾黄色报警");
  //   } else {
  //     relay_set_state(RELAY_OFF);
  //     log_i("RELAY", "🔧 风扇已关闭 - 报警解除或不需要风扇");
  //   }
  //   last_fan_state = current_fan_state;
  // }

  // 蜂鸣器控制：只在报警状态变化时停止蜂鸣器
  if (any_alarm_active != last_alarm_state) {
    if (!any_alarm_active) {
      // 报警解除：停止蜂鸣器
      Buzzer_Stop();
      log_i("BUZZER", "🔇 所有报警已解除，蜂鸣器停止");
    } else {
      // 报警激活：蜂鸣器由上面的Buzzer_Alarm调用控制
      log_d("BUZZER", "报警激活，蜂鸣器开始工作");
    }
    last_alarm_state = any_alarm_active;
  }
}

// 按键事件处理函数
static void handle_key_event(uint8_t key_index, key_event_t event) {
  switch (event) {
    case KEY_EVENT_PRESS:
      // 按键按下：驱动舵机到指定角度
      if (key_index == 0) {
        servo_set_angle(SERVO_ANGLE_MIN);    // KEY1 → 舵机 0°
      } else if (key_index == 1) {
        servo_set_angle(SERVO_ANGLE_MAX);    // KEY2 → 舵机 180°
      }
      break;
    case KEY_EVENT_RELEASE:
      // log_i("KEY", "按键%d释放", key_index + 1);
      // 舵机位置由 KEY_EVENT_PRESS 设定，释放无需额外处理
      break;

    case KEY_EVENT_SHORT_PRESS:
      log_i("KEY", "按键%d短按", key_index + 1);
      //  按键1短按：切换LED状态0
      if (key_index == 0) {
        Buzzer_Alarm(1, 100, 0);
      }
      // 按键2短按：切换电机启停
      if (key_index == 1) {
        Buzzer_Alarm(2, 500, 50);
      }
      break;

    case KEY_EVENT_LONG_PRESS:
      log_i("KEY", "按键%d长按", key_index + 1);
      // 按键1长按：清空KV存储
      if (key_index == 0) {
        log_i("KEY", "按键1长按：开始清空KV存储...");
        // 清空所有KV存储数据
        kv_clear_all();
        log_i("KEY", "✅ KV存储已清空");
        // 蜂鸣器提示音表示操作完成
        Buzzer_Alarm(1, 1000, 50);
      }
      // 按键2长按：关闭所有报警
      if (key_index == 1) {
        log_i("KEY", "按键2长按：开始关闭所有报警...");
        // 清除所有类型的报警
        alarm_system_clear_alarm(ALARM_TYPE_TEMPERATURE);
        alarm_system_clear_alarm(ALARM_TYPE_HUMIDITY);
        alarm_system_clear_alarm(ALARM_TYPE_SMOKE);

        // 强制重置所有报警状态
        for (int i = 0; i < 3; i++) {
          g_alarm_active[i] = false;
          g_current_alarm_level[i] = ALARM_LEVEL_NONE;
        }
        g_running_status = 0;

        // 停止蜂鸣器和LED报警
        Buzzer_Stop();
        LED_OFF();
        // relay_set_state(RELAY_OFF);  // 继电器已注释

        log_i("KEY", "✅ 所有报警已关闭，系统状态重置为正常");
        // 蜂鸣器提示音表示操作完成
        Buzzer_Alarm(2, 200, 100);
      }
      break;

    case KEY_EVENT_DOUBLE_CLICK:
      // log_i("KEY", "按键%d双击", key_index + 1);
      //  按键1双击：LED闪烁
      if (key_index == 0) {
      }
      // 按键2双击：显示系统信息
      if (key_index == 1) {
      }
      break;

    case KEY_EVENT_HOLD:
      // 舵机位置由 KEY_EVENT_PRESS 设定，保持按下无需额外处理
      break;

    default:
      break;
  }
}

// 按键定时器回调函数
void KeyTimerCb(void* arg) {
  (void)arg;

  if (g_key_manager != NULL) {
    // 更新按键状态
    key_manager_update(g_key_manager);

    // 检查所有按键的事件
    for (uint8_t i = 0; i < g_key_manager->key_count; i++) {
      key_event_t event = key_manager_get_event(g_key_manager, i);
      if (event != KEY_EVENT_NONE) {
        handle_key_event(i, event);
      }
    }
  }
}

// 全局WiFi配置变量 - 移动到文件顶部
static char g_wifi_ssid[32] = "";
static char g_wifi_password[64] = "";
static bool g_ap_created = false;
bool g_wifi_connected = false;
char g_wifi_ip[16] = "";  // IP地址存储
static const char* g_ap_ssid = "Hi3861_Config_AP";
static const char* g_ap_password = "12345678";

// 全局STA连接重试相关变量
static int g_sta_retry_count = 0;
#define MAX_STA_RETRY_COUNT 3
#define STA_RETRY_INTERVAL_MS 5000     // 5秒重试间隔
#define WIFI_CONNECT_TIMEOUT_MS 30000  // 30秒连接超时

// 重构后的网络任务函数 - 简化逻辑，取消STA/AP切换
void network_task(void* arg) {
  (void)arg;

  log_i("NETWORK", "网络任务开始启动");

  // 等待系统初始化完成
  sleep(2);
  log_i("NETWORK", "系统初始化完成，开始WiFi配置");

  Time_DelayMs(1000);  // 等待WiFi模块初始化

  // 尝试从KV存储读取WiFi配置到全局变量
  bool has_sta_config = false;
  if (kv_get_string("wifi_ssid", g_wifi_ssid, sizeof(g_wifi_ssid)) ==
          KV_SUCCESS &&
      kv_get_string("wifi_password", g_wifi_password,
                    sizeof(g_wifi_password)) == KV_SUCCESS &&
      strlen(g_wifi_ssid) > 0) {
    log_i("NETWORK", "从KV存储读取到WiFi配置: %s", g_wifi_ssid);
    has_sta_config = true;
  } else {
    log_i("NETWORK", "KV存储中没有找到WiFi配置或配置为空");
    has_sta_config = false;
  }

  // 重构逻辑：有STA配置且有效时启动STA模式，否则启动AP模式
  if (has_sta_config) {
    log_i("NETWORK", "启动STA模式，尝试连接到WiFi网络: %s", g_wifi_ssid);

    // 尝试连接STA网络
    WifiErrorCode wifi_ret = WiFi_connectHotspots(g_wifi_ssid, g_wifi_password);

    if (wifi_ret == WIFI_SUCCESS) {
      log_i("NETWORK", "STA连接成功");
      g_wifi_connected = true;
      g_sta_retry_count = 0;  // 连接成功，重置重试计数器

      // 获取IP地址
      char* ip_addr = WiFi_GetLocalIP();
      if (ip_addr != NULL) {
        strncpy(g_wifi_ip, ip_addr, sizeof(g_wifi_ip) - 1);
        log_i("NETWORK", "获取到IP地址: %s", g_wifi_ip);
      }

      // STA连接成功，不创建AP模式
      g_ap_created = false;
    } else {
      log_w("NETWORK", "STA连接失败，错误码: %d", wifi_ret);
      g_sta_retry_count = 1;  // 开始重试计数
    }
  } else {
    // 没有STA配置，直接启动AP模式
    log_i("NETWORK", "没有STA配置，启动AP模式");
    g_ap_created = true;
    g_wifi_connected = false;
  }

  // 如果STA连接失败，启动AP模式
  if (!g_wifi_connected && has_sta_config) {
    log_i("NETWORK", "STA连接失败，启动AP模式");
    g_ap_created = true;
  }

  // 创建AP模式（如果需要）
  if (g_ap_created && !g_wifi_connected) {
    log_i("NETWORK", "启动AP模式");
    WifiErrorCode ap_ret = WiFi_createHotspots(g_ap_ssid, g_ap_password);
    if (ap_ret == WIFI_SUCCESS) {
      log_i("NETWORK", "AP热点创建成功: %s", g_ap_ssid);
      g_ap_created = true;

      // 获取AP的IP地址
      char* ip_addr = WiFi_GetLocalIP();
      if (ip_addr != NULL) {
        strncpy(g_wifi_ip, ip_addr, sizeof(g_wifi_ip) - 1);
        log_i("NETWORK", "AP热点IP地址: %s", g_wifi_ip);
      }
    } else {
      log_e("NETWORK", "AP热点创建失败，错误码: %d", ap_ret);
      // AP创建失败，等待一段时间后重试
      Time_DelayMs(3000);
    }
  }

  log_i("NETWORK", "网络任务初始化完成，进入主循环");

  // 重构后的网络任务主循环
  static uint32_t g_last_network_check = 0;
  static uint32_t g_last_php_update = 0;
  static uint32_t last_sta_retry_time = 0;

  while (1) {
    uint32_t current_time_ms = Time_GetCurrentMs();

    // 重构：STA重试逻辑（仅在AP模式下重试STA）
    if (g_ap_created && has_sta_config && !g_wifi_connected &&
        g_sta_retry_count > 0 && g_sta_retry_count <= MAX_STA_RETRY_COUNT) {
      if (current_time_ms - last_sta_retry_time >= STA_RETRY_INTERVAL_MS) {
        log_i("NETWORK", "开始第%d次STA连接重试（最多%d次）", g_sta_retry_count,
              MAX_STA_RETRY_COUNT);

        WifiErrorCode wifi_ret =
            WiFi_connectHotspots(g_wifi_ssid, g_wifi_password);

        if (wifi_ret == WIFI_SUCCESS) {
          log_i("NETWORK", "STA重试连接成功");
          g_wifi_connected = true;
          g_sta_retry_count = 0;
          g_ap_created = false;  // STA成功，关闭AP模式

          // 获取IP地址
          char* ip_addr = WiFi_GetLocalIP();
          if (ip_addr != NULL) {
            strncpy(g_wifi_ip, ip_addr, sizeof(g_wifi_ip) - 1);
            log_i("NETWORK", "获取到IP地址: %s", g_wifi_ip);
          }
        } else {
          log_w("NETWORK", "STA重试连接失败，错误码: %d", wifi_ret);
          g_sta_retry_count++;

          // 重构：达到最大重试次数后，删除KV配置并重启
          if (g_sta_retry_count > MAX_STA_RETRY_COUNT) {
            log_i("NETWORK", "STA连接重试达到最大次数%d次，删除KV配置并重启",
                  MAX_STA_RETRY_COUNT);

            // 删除KV存储中的WiFi配置
            kv_delete("wifi_ssid");
            kv_delete("wifi_password");
            log_i("NETWORK", "✅ KV存储中的WiFi配置已删除");

            // 等待1秒后重启系统
            Time_DelayMs(1000);
            log_i("NETWORK", "执行系统重启...");
            RebootSystem();
          }
        }
        last_sta_retry_time = current_time_ms;
      }
    }

    // 网络状态检查（10秒间隔）
    if (current_time_ms - g_last_network_check >= NETWORK_CHECK_INTERVAL_MS) {
      g_last_network_check = current_time_ms;

      if (g_ap_created) {
        log_i("NETWORK", "网络状态: AP模式运行中, IP: %s", g_wifi_ip);
        if (has_sta_config) {
          log_i("NETWORK", "          STA重试次数: %d/%d", g_sta_retry_count,
                MAX_STA_RETRY_COUNT);
        }
      } else if (g_wifi_connected) {
        log_i("NETWORK", "网络状态: STA连接成功, IP: %s", g_wifi_ip);
      } else {
        log_i("NETWORK", "网络状态: 未连接");
      }
    }

    // PHP更新逻辑
    if (current_time_ms - g_last_php_update >= PHP_UPDATE_INTERVAL_MS) {
      g_last_php_update = current_time_ms;
      php_api_update_web_server();
    }

    // 添加短延时，让出CPU时间
    Time_DelayMs(10);
  }
}

// LED初始化函数 - 设置初始状态为关闭
void led_init(void) {
  hi_io_set_func(LED_PIN, LED_GPIO_FUN);
  hi_gpio_set_dir(LED_PIN, HI_GPIO_DIR_OUT);
  LED_OFF();  // 初始状态设置为关闭
  log_i("LED", "LED初始化完成，初始状态：关闭");
}

// WIFI配置解析函数 - 添加清除NDEF数据功能
// WIFI配置解析函数 - 修改为在成功写入KV存储后清除NDEF数据
static void parse_wifi_config(const char* wifi_uri) {
  char ssid[64] = {0};
  char password[64] = {0};
  char auth_type[32] = {0};

  // 跳过"WIFI:"前缀
  const char* ptr = wifi_uri + 5;

  while (*ptr != '\0') {
    // 查找分隔符
    if (*ptr == ';') {
      ptr++;
      continue;
    }

    // 解析字段
    if (*ptr == 'S' && *(ptr + 1) == ':') {
      // 解析SSID
      const char* start = ptr + 2;
      int i = 0;
      while (*start != ';' && *start != '\0') {
        ssid[i++] = *start++;
      }
      ssid[i] = '\0';
      log_i("NFC", "检测到SSID: %s", ssid);
      ptr = start;
    } else if (*ptr == 'T' && *(ptr + 1) == ':') {
      // 解析认证类型
      const char* start = ptr + 2;
      int i = 0;
      while (*start != ';' && *start != '\0') {
        auth_type[i++] = *start++;
      }
      auth_type[i] = '\0';
      log_i("NFC", "认证类型: %s", auth_type);
      ptr = start;
    } else if (*ptr == 'P' && *(ptr + 1) == ':') {
      // 解析密码
      const char* start = ptr + 2;
      int i = 0;
      while (*start != ';' && *start != '\0') {
        password[i++] = *start++;
      }
      password[i] = '\0';

      // 显示密码长度（不显示明文）
      log_i("NFC", "密码长度: %d 字符", i);
      ptr = start;
    } else {
      ptr++;
    }
  }

  log_i("NFC", "WIFI配置解析完成:");
  log_i("NFC", "   WIFI名称: %s", ssid);
  log_i("NFC", "   加密方式: %s", auth_type);
  // 将配置写入KV存储
  if (strlen(ssid) > 0) {
    kv_set_string("wifi_ssid", ssid);
    log_i("NFC", "✅ SSID已保存到KV存储");  // 从NFC读取配置数据

    if (strlen(password) > 0) {
      kv_set_string("wifi_password", password);
      log_i("NFC", "✅ 密码已保存到KV存储");
    } else {
      kv_set_string("wifi_password", "");
      log_i("NFC", "⚠ 开放网络，无需密码");
    }

    // 清除NDEF数据，确保下一次读取是空
    log_i("NFC", "正在清除NDEF数据...");
    if (nfc_clear_ndef_data()) {
      log_i("NFC", "✅ NDEF数据清除成功，确保下次读取为空");
    } else {
      log_e("NFC", "❌ NDEF数据清除失败");
    }

    // 添加一条空tag（初始化第一页为NDEF格式）
    uint8_t empty_tag_data[NFC_PAGE_SIZE] = {0};
    empty_tag_data[0] = 0x03;  // NDEF起始标志
    empty_tag_data[1] = 0x00;  // 记录结束位置（空记录）
    empty_tag_data[2] = 0xFE;  // NDEF结束标志

    log_i("NFC", "添加空NFC标签...");
    if (NT3HWriteUserData(0, empty_tag_data)) {
      log_i("NFC", "✅ 空NFC标签添加成功，确保下次读取为空");
    } else {
      log_e("NFC", "❌ 空NFC标签添加失败");
      return HI_ERR_FAILURE;
    }

    // LED快速闪烁5次表示检测到WIFI配置
    for (int j = 0; j < 5; j++) {
      LED_ON();
      Time_DelayMs(50);
      LED_OFF();
      Time_DelayMs(50);
    }

    log_i("NFC", "🎉 WIFI配置已保存，NDEF数据已清除，可重新启动网络连接");
  } else {
    log_e("NFC", "❌ SSID为空，无法保存配置");
  }
}

// 改进的NFC检测函数 - 基于template.c的实现
static bool check_nfc_tag(void) {
  if (!g_nfc_initialized) {
    log_e("NFC", "❌ NFC模块未初始化，无法检测标签");
    return false;
  }

  log_i("NFC", "开始检测NFC标签...");

  int max_attempts = 3;
  for (int attempt = 1; attempt <= max_attempts; attempt++) {
    log_i("NFC", "尝试 %d/%d，请将NFC标签靠近开发板...", attempt, max_attempts);

    uint8_t ndefLen = 0;
    uint8_t ndef_Header = 0;
    uint32_t result_code = 0;

    // 尝试读取NDEF头信息
    result_code = NT3HReadHeaderNfc(&ndefLen, &ndef_Header);

    if (result_code == true) {
      log_i("NFC", "✅ NFC标签检测成功！");
      log_i("NFC", "   NDEF数据长度: %d 字节", ndefLen);

      // 尝试读取NDEF数据
      if (ndefLen > 0) {
        uint32_t total_len = ndefLen + NDEF_HEADER_SIZE;  // 加上头大小
        log_i("NFC", "读取NDEF数据，总长度: %d 字节", total_len);
        uint8_t* ndefBuff = (uint8_t*)malloc(total_len + 1);
        if (ndefBuff != NULL) {
          result_code = get_NDEFDataPackage(ndefBuff, total_len);
          if (result_code == HI_ERR_SUCCESS) {
            log_i("NFC", "✅ NDEF数据读取成功");

            // 显示原始数据用于调试
            log_i("NFC", "原始NDEF数据预览:");
            char debug_str[51] = {0};
            int debug_idx = 0;
            for (uint32_t i = 0; i < total_len && i < 50; i++) {
              if (ndefBuff[i] >= 32 && ndefBuff[i] <= 126) {
                debug_str[debug_idx++] = ndefBuff[i];
              } else {
                debug_str[debug_idx++] = '.';
              }
            }
            debug_str[debug_idx] = '\0';
            log_i("NFC", "   内容: %s", debug_str);

            // 查找WIFI:前缀
            int wifi_found = 0;
            for (uint32_t i = 0; i < total_len - 5 && !wifi_found; i++) {
              if (memcmp(&ndefBuff[i], "WIFI:", 5) == 0) {
                wifi_found = 1;

                // 提取整个WIFI配置字符串
                char wifi_config[256] = {0};
                int j = 0;
                // 从WIFI:开始复制
                for (uint32_t k = i; k < total_len && j < 255; k++) {
                  wifi_config[j++] = ndefBuff[k];

                  // 检查是否到达配置结束（两个分号）
                  if (k > i + 10 && ndefBuff[k] == ';' && k + 1 < total_len &&
                      ndefBuff[k + 1] == ';') {
                    wifi_config[j++] = ';';  // 添加第二个分号
                    wifi_config[j] = '\0';   // 结束字符串
                    break;
                  }
                }
                if (j > 0) {
                  wifi_config[j] = '\0';
                  log_i("NFC", "🎉 检测到WIFI配置: %s", wifi_config);

                  // 解析并保存WIFI配置
                  parse_wifi_config(wifi_config);
                } else {
                  log_e("NFC", "❌ 无法提取WIFI配置字符串");
                }

                break;
              }
            }
            if (!wifi_found) {
              log_w("NFC", "⚠ 未检测到WIFI配置");
              // 显示更多调试信息
              log_i("NFC", "原始数据十六进制:");
              for (uint32_t i = 0; i < (total_len < 32 ? total_len : 32); i++) {
                if (i % 16 == 0) log_i("NFC", "  ");
                log_i("NFC", "%02X ", ndefBuff[i]);
              }
              log_i("NFC", "");
            }
          } else {
            log_e("NFC", "❌ NDEF数据读取失败，错误码: 0x%08X", result_code);
          }

          free(ndefBuff);
        } else {
          log_e("NFC", "❌ 内存分配失败");
        }
      } else {
        log_w("NFC", "⚠ 标签中没有NDEF数据或数据长度为0");
      }

      log_i("NFC", "✅ 读取成功，等待下一个标签...");

      // LED长亮1秒表示成功
      LED_ON();
      Time_DelayMs(1000);
      LED_OFF();

      return true;  // 检测成功，退出
    } else {
      log_i("NFC", "❌ 第 %d 次尝试失败", attempt);

      // 等待后重试
      if (attempt < max_attempts) {
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

  return false;  // 检测失败
}

void Main_Task(void* arg) {
  (void)arg;
  uint32_t current_time_ms = 0;
  // 数据获取类时间戳（2秒间隔）
  uint32_t g_last_data_acquisition = Time_GetCurrentMs();
  // 设备控制类时间戳（5秒间隔）
  uint32_t g_last_device_control = Time_GetCurrentMs();
  // 系统状态类时间戳（30秒间隔）
  uint32_t g_last_system_status = Time_GetCurrentMs();
  // 传感器数据发送时间戳（1秒间隔）- 新增
  uint32_t g_last_sensor_send = Time_GetCurrentMs();

  uint8_t g_last_alarm_update = Time_GetCurrentMs();

  log_i("MAIN", "主任务开始运行");

  // 主循环
  while (1) {
    current_time_ms = Time_GetCurrentMs();

    // ==================== 持续报警处理（每次循环都检查） ====================
    handle_continuous_alarm();

    if (current_time_ms - g_last_alarm_update >= 25) {
      g_last_alarm_update = current_time_ms;
      // 使用报警系统库更新传感器数据
      float temperature = (float)g_temperature;
      float humidity = (float)g_humidity;
      float smoke = (float)g_smoke_data.raw_value;  // 使用原始ADC值作为烟雾浓度

      // 调用报警系统更新函数
      bool has_alarm = alarm_system_update(temperature, humidity, smoke);

      if (has_alarm) {
        log_w("ALARM_SYSTEM", "检测到报警事件");
      }
    }

    // ==================== 数据获取类操作（2秒间隔） ====================
    if (current_time_ms - g_last_data_acquisition >= 1000) {
      g_last_data_acquisition = current_time_ms;
      // log_i("DATA_ACQUISITION", "开始数据采集周期");

      // 1. DHT11温湿度传感器读取
      if (dht11_read_data(&g_temperature, &g_humidity) == 0) {
        log_i("DHT11", "温湿度传感器: 湿度=%d%%, 温度=%d°C", g_humidity,
              g_temperature);
      } else {
        log_e("DHT11", "传感器读取失败");
      }
      // ========== OLED 传感器显示已注释，改为后台屏幕测试 ==========
      // 需要 OLED 显示时取消注释下方代码块
      /* OLED_Clear();

      // 构建报警状态字符串
      char alarm_str[48] = "";
      const char* alarm_names[] = {"T", "H", "S"};
      for (int i = 0; i < 3; i++) {
        if (g_alarm_active[i]) {
          char lvl = 'N';
          switch (g_current_alarm_level[i]) {
            case ALARM_LEVEL_YELLOW: lvl = 'Y'; break;
            case ALARM_LEVEL_RED:    lvl = 'R'; break;
            default:                 lvl = 'N'; break;
          }
          char tmp[16];
          snprintf(tmp, sizeof(tmp), "%s:%c ", alarm_names[i], lvl);
          strncat(alarm_str, tmp, sizeof(alarm_str) - strlen(alarm_str) - 1);
        }
      }

      if (strlen(alarm_str) > 0) {
        OLED_Printf(8, "T:%.1fC\nH:%d%%\nS:%d\n%s", (float)g_temperature,
                    g_humidity, g_smoke_data.raw_value, alarm_str);
      } else {
        OLED_Printf(8, "T:%.1fC\nH:%d%%\nS:%d", (float)g_temperature,
                    g_humidity, g_smoke_data.raw_value);
      } */
    }

    // ==================== 传感器数据发送（1秒间隔）- 新增 ====================
    if (current_time_ms - g_last_sensor_send >= 1000) {
      g_last_sensor_send = current_time_ms;

      // 检查网络连接状态，只有在WiFi已连接时才发送数据
      if (!g_wifi_connected) {
        static uint32_t last_network_warn = 0;
        if (current_time_ms - last_network_warn >= 10000)  // 每10秒警告一次
        {
          log_w("SENSOR_SEND", "⚠ 网络未连接，跳过传感器数据发送");
          last_network_warn = current_time_ms;
        }
      } else {
        // log_i("SENSOR_SEND", "开始传感器数据发送周期");

        // 创建传感器数据结构体并填充数据
        sensor_data_t sensor_data = {0};
        sensor_data.temperature = (float)g_temperature;
        sensor_data.humidity = (float)g_humidity;
        sensor_data.smoke = g_smoke_data.raw_value;  // 使用电压值作为烟雾浓度
        sensor_data.relay_status = relay_get_state();
        sensor_data.buzzer_status = g_smoke_alarm_triggered;

        // 发送传感器数据到服务器
        php_api_result_t send_result = php_api_send_sensor_data(&sensor_data);

        // 简化日志输出，每30次发送记录一次
        static uint32_t send_count = 0;
        static uint32_t success_count = 0;
        static uint32_t fail_count = 0;

        send_count++;

        if (send_result == PHP_API_SUCCESS) {
          success_count++;
        } else {
          fail_count++;
        }

        // 每30次发送记录一次日志
        if (send_count % 30 == 0) {
          log_i("SENSOR_SEND",
                "📊 传感器数据发送统计: 总发送=%d次, 成功=%d次, 失败=%d次, "
                "成功率=%.1f%%",
                send_count, success_count, fail_count,
                send_count > 0 ? (float)success_count / send_count * 100 : 0);
        }
      }
    }

    // ==================== 设备控制类操作（5秒间隔） ====================
    if (current_time_ms - g_last_device_control >= 5000) {
      g_last_device_control = current_time_ms;
      log_i("DEVICE_CONTROL", "开始设备控制周期");

      // 1. 烟雾传感器读取和报警
      smoke_sensor_read_data(&g_smoke_data);
      log_i("SMOKE", "烟雾传感器: ADC值=%d, 电压=%.2fV, 等级=%s",
            g_smoke_data.raw_value, g_smoke_data.voltage,
            smoke_sensor_get_level_string(g_smoke_data.level));

      // 2. NFC标签检测
      if (g_nfc_initialized) {
        check_nfc_tag();
      } else {
        log_w("NFC", "NFC模块未初始化，跳过检测");
      }

      log_i("DEVICE_CONTROL", "设备控制周期完成");
    }

    // ==================== 系统状态类操作（30秒间隔） ====================
    if (current_time_ms - g_last_system_status >= 30000) {
      g_last_system_status = current_time_ms;
      log_i("SYSTEM_STATUS", "开始系统状态更新周期");

      // 1. 网络状态显示
      if (g_wifi_connected) {
        log_i("NETWORK", "WiFi状态: 已连接 %s, IP: %s", g_wifi_ssid, g_wifi_ip);
      } else {
        log_i("NETWORK", "网络状态: AP模式运行中, IP: %s", g_wifi_ip);
      }

      log_i("SYSTEM_STATUS", "系统状态更新周期完成");
    }

    // 添加短延时，让出CPU时间
    Time_DelayMs(10);
  }
}

/**
 * @brief MG995 舵机测试
 *        从 0° 递增到 180°，再递减回 0°，验证全行程
 */
static void servo_test(void)
{
    log_i("SERVO", "========== MG995 舵机全行程测试开始 ==========");

    // 0° → 180°，每 30° 一步
    for (uint16_t a = 0; a <= 180; a += 30) {
        log_i("SERVO", "  舵机转到 %d°", a);
        servo_set_angle(a);
        Time_DelayMs(600);
    }
    // 180° → 0°，每 30° 一步
    for (uint16_t a = 150; a > 0; a -= 30) {
        log_i("SERVO", "  舵机转到 %d°", a);
        servo_set_angle(a);
        Time_DelayMs(600);
    }
    servo_set_angle(0);
    Time_DelayMs(600);

    log_i("SERVO", "========== MG995 舵机全行程测试完成 ✅ 归位 0° ==========");
}

/**
 * @brief MG995 舵机初始化（GPIO5 输出）
 */
static void servo_init(void)
{
    hi_gpio_init();
    hi_io_set_func(SERVO_PIN, SERVO_GPIO_FUN);
    hi_gpio_set_dir(SERVO_PIN, HI_GPIO_DIR_OUT);
    g_servo_angle = 90;
    hi_gpio_set_ouput_val(SERVO_PIN, HI_GPIO_VALUE0);
}

/**
 * @brief 设置舵机角度（0~180°）
 *        MG995 标准 50Hz：0°→500us，180°→2500us 脉宽，周期 20ms
 *        继续发送多拍以保证大扭矩舵机充分到位并保持
 */
static void servo_set_angle(uint16_t angle)
{
    if (angle > SERVO_ANGLE_MAX) {
        angle = SERVO_ANGLE_MAX;
    }
    if (angle > 0 && angle < SERVO_ANGLE_MIN) {
        angle = SERVO_ANGLE_MIN;
    }

    g_servo_angle = angle;

    /* 角度→脉宽(us)：0°→500us  180°→2500us */
    uint32_t time = (uint32_t)(angle - SERVO_ANGLE_MIN) * (2500 - 500) /
                    (SERVO_ANGLE_MAX - SERVO_ANGLE_MIN) + 500;

    for (uint8_t i = 0; i < SERVO_PULSES; i++) {
        hi_gpio_set_ouput_val(SERVO_PIN, HI_GPIO_VALUE1);
        hi_udelay(time);
        hi_gpio_set_ouput_val(SERVO_PIN, HI_GPIO_VALUE0);
        hi_udelay(SERVO_PERIOD_US - time);
    }

    log_i("SERVO", "MG995 角度 = %d° (脉宽 %uus)", angle, time);
}

static void Main_Entry(void) {
  log_i("SYSTEM", "系统初始化开始");
  // 初始化库
  Time_Init();
  led_init();
  Buzzer_Init();
  smoke_sensor_init();
  lightsense_init();
  // relay_init();  // 继电器已注释（GPIO8 腾给步进电机）
  servo_init();  // 舵机初始化（GPIO5）

  // === 舵机测试 ===
  servo_test();
  // ================

  // NFC初始化 - 改进初始化逻辑，基于template.c
  log_i("NFC", "正在初始化NFC模块...");

  uint32_t nfc_ret = nfc_init();
  Time_DelayMs(500);
  if (nfc_ret == HI_ERR_SUCCESS) {
    g_nfc_initialized = true;
    log_i("NFC", "✅ NFC模块初始化成功");
    log_i("NFC", "请将包含WIFI配置的NFC标签靠近开发板");
    log_i("NFC", "支持的格式: WIFI:S:SSID;T:TYPE;P:PASSWORD;;");
    log_i("NFC", "NFC检测将每5秒执行一次");

    // 立即进行一次NFC检测
    log_i("NFC", "执行首次NFC检测...");
    if (check_nfc_tag()) {
      log_i("NFC", "首次NFC检测完成");
    } else {
      log_i("NFC", "首次NFC检测未发现标签");
    }
  } else {
    log_e("NFC", "❌ NFC模块初始化失败，错误码: 0x%08X", nfc_ret);
    log_e("NFC", "请检查NFC模块连接和配置");
    g_nfc_initialized = false;
  }

  // 初始化按键管理器
  log_i("KEY", "正在初始化按键管理器...");
  g_key_manager = key_manager_create(2);  // 最多支持2个按键
  if (g_key_manager != NULL) {
    // 添加按键1 (GPIO11)
    int8_t key1_index =
        key_manager_add_key(g_key_manager, KEY1_PIN, 0, 10, 1000, 300);
    if (key1_index >= 0) {
      log_i("KEY", "按键1 (GPIO13) 添加成功，索引: %d", key1_index);
    } else {
      log_e("KEY", "按键1添加失败");
    }

    // 添加按键2 (GPIO12)
    int8_t key2_index =
        key_manager_add_key(g_key_manager, KEY2_PIN, 0, 10, 1000, 300);
    if (key2_index >= 0) {
      log_i("KEY", "按键2 (GPIO12) 添加成功，索引: %d", key2_index);
    } else {
      log_e("KEY", "按键2添加失败");
    }

    // 创建按键定时器（5ms周期）
    g_key_timer = osTimerNew(KeyTimerCb, osTimerPeriodic, NULL, NULL);
    if (g_key_timer != NULL) {
      osTimerStart(g_key_timer, 10);
      log_i("KEY", "按键定时器启动成功 (10ms周期)");
    } else {
      log_e("KEY", "按键定时器创建失败");
    }
  } else {
    log_e("KEY", "按键管理器创建失败");
  }

  // 初始化KV存储模块
  kv_result_t kv_ret = kv_init();
  if (kv_ret == KV_SUCCESS) {
    log_i("KV", "KV存储模块初始化成功");
    // KV断电保存验证测试
    log_i("KV_TEST", "开始KV断电保存验证测试...");

    // 检查启动计数（使用uint32 API）
    uint32_t boot_count = 0;
    kv_result_t boot_count_result = kv_get_uint32("boot_count", &boot_count);

    if (boot_count_result == KV_ERROR_KEY_NOT_FOUND) {
      // 第一次运行：设置初始数据
      log_i("KV_TEST", "检测到第一次运行，设置初始KV数据...");

      // 设置启动计数为1（使用uint32 API）
      kv_set_uint32("boot_count", 1);
      boot_count = 1;

      // 设置测试字符串
      const char* test_string = "Hi3861_KV_Test_FirstBoot";
      kv_set_string("test_key", test_string);

      // 设置时间戳（使用uint32 API）
      uint32_t current_time = Time_GetCurrentMs();
      kv_set_uint32("first_boot_time", current_time);

      // 设置设备信息
      const char* device_info = "Hi3861_WiFi_IoT_Device_v1.0_FirstBoot";
      kv_set_string("device_info", device_info);

      log_i("KV_TEST", "✅ 第一次运行：KV数据初始化完成");
      log_i("KV_TEST", "   测试字符串: %s", test_string);
      log_i("KV_TEST", "   启动时间: %u ms", current_time);
      log_i("KV_TEST", "   设备信息: %s", device_info);
      log_i("KV_TEST", "💡 请断电重启设备，验证KV数据是否保存");
    } else if (boot_count_result == KV_SUCCESS) {
      // 后续运行：验证数据是否保存
      log_i("KV_TEST", "检测到后续运行，验证KV数据保存...");

      // 更新启动计数（使用uint32 API）
      boot_count++;
      kv_set_uint32("boot_count", boot_count);

      // 验证测试字符串
      char test_buffer[64] = {0};
      kv_result_t test_result =
          kv_get_string("test_key", test_buffer, sizeof(test_buffer));
      if (test_result == KV_SUCCESS) {
        log_i("KV_TEST", "✅ 测试字符串验证成功: %s", test_buffer);
      } else {
        log_e("KV_TEST", "❌ 测试字符串验证失败，错误码: %d", test_result);
      }

      // 验证启动时间（使用uint32 API）
      uint32_t first_boot_time = 0;
      kv_result_t time_result =
          kv_get_uint32("first_boot_time", &first_boot_time);
      if (time_result == KV_SUCCESS) {
        log_i("KV_TEST", "✅ 首次启动时间验证成功: %u ms", first_boot_time);
      } else {
        log_e("KV_TEST", "❌ 首次启动时间验证失败，错误码: %d", time_result);
      }

      // 验证设备信息
      char device_buffer[64] = {0};
      kv_result_t device_result =
          kv_get_string("device_info", device_buffer, sizeof(device_buffer));
      if (device_result == KV_SUCCESS) {
        log_i("KV_TEST", "✅ 设备信息验证成功: %s", device_buffer);
      } else {
        log_e("KV_TEST", "❌ 设备信息验证失败，错误码: %d", device_result);
      }

      // 显示启动次数
      log_i("KV_TEST", "📊 当前启动次数: %u", boot_count);

      // 验证中文支持
      char chinese_buffer[64] = {0};
      kv_result_t chinese_result =
          kv_get_string("chinese_test", chinese_buffer, sizeof(chinese_buffer));
      if (chinese_result == KV_SUCCESS) {
        log_i("KV_TEST", "✅ 中文字符串验证成功: %s", chinese_buffer);
      } else if (chinese_result == KV_ERROR_KEY_NOT_FOUND) {
        // 如果是第一次看到中文测试失败，可能是之前没有设置，现在设置一下
        const char* chinese_test = "KV存储测试-中文验证";
        kv_set_string("chinese_test", chinese_test);
        log_i("KV_TEST", "💡 设置中文字符串: %s", chinese_test);
      } else {
        log_e("KV_TEST", "❌ 中文字符串验证失败，错误码: %d", chinese_result);
      }

      log_i("KV_TEST", "🎉 KV断电保存验证完成！");
    } else {
      log_e("KV_TEST", "❌ 启动计数读取失败，错误码: %d", boot_count_result);
    }

    // 初始化默认配置（使用uint32 API）
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

  // 初始化PHP API模块
  log_i("PHP_API", "正在初始化PHP API模块...");
  php_api_result_t php_ret = php_api_init(0);
  if (php_ret == PHP_API_SUCCESS) {
    log_i("PHP_API", "✅ PHP API模块初始化成功");
    // 启动PHP网页服务器
    php_ret = php_api_start_web_server();
    if (php_ret == PHP_API_SUCCESS) {
      log_i("PHP_API", "✅ PHP网页服务器启动成功");
      log_i("PHP_API", "📡 网页服务器地址: http://192.168.0.1:80");
    } else {
      log_e("PHP_API", "❌ PHP网页服务器启动失败，错误码: %d", php_ret);
    }
  } else {
    log_e("PHP_API", "❌ PHP API模块初始化失败，错误码: %d", php_ret);
  }

  // 初始化DHT11
  log_i("DHT11", "正在初始化DHT11传感器...");
  uint8_t retry_count = 0;
  while (dht11_init()) {
    log_w("DHT11", "初始化失败，正在重试... (第%d次/共%d次)", retry_count + 1,
          DHT11_RETRY_COUNT);
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

  OLED_Init(1);
  OLED_Invert(false);
  OLED_SetContrast(255);
  OLED_Refresh();

  // ========== 显示开机 Logo ==========
  OLED_ShowBitmap(0, 0, 128, 64, gImage_logo);
  Time_DelayMs(3000);  // 显示 3 秒

  // ========== OLED 屏幕测试（5分钟完整测试） ==========

  // -------------------- 测试1：逐字打印 + 续接 --------------------
  OLED_Clear();
  char text[] = "Welcome to OLED Test!\nThis is a long message\nfor testing word wrap\nand line continuation!\nLets see how it handles\nmulti-line text!\n";
  int text_len = (int)strlen(text);
  for (int i = 0; i < text_len; i++) {
    OLED_Printf(8, "%c", text[i]);
    Time_DelayMs(50);
  }
  OLED_Printf(8, "Done!");
  Time_DelayMs(1000);

  /*
  // -------------------- 测试2：\t 对齐 + 刷新率测试 --------------------
  OLED_Clear();
  OLED_Printf(8, "FPS Test...");
  Time_DelayMs(1000);

  uint32_t frame_cnt = 0;
  uint32_t fps_t0 = Time_GetCurrentMs();
  while (Time_GetCurrentMs() - fps_t0 < 10000) {  // 测10秒
    OLED_Clear();
    frame_cnt++;

    // 动态内容：不同帧显示不同行数，让渲染时间有波动
    uint32_t t = Time_GetCurrentMs() - fps_t0;
    uint32_t phase = (t / 2000) % 3;  // 每2秒切换一次模式

    if (phase == 0) {
      // 模式A：单行简单
      OLED_Printf(8, "Frame %lu  t=%lu", frame_cnt, t);
    } else if (phase == 1) {
      // 模式B：表格多行
      OLED_Printf(8, "Frame:%lu\nCol1\tCol2\tCol3\nA\t16\t32\nB\t64\t128", frame_cnt);
    } else {
      // 模式C：长文本 + 数字混合
      OLED_Printf(8, "F:%lu  %lums\nData:%lu %lu %lu\nTest:\tOK\tPASS",
                  frame_cnt, t, frame_cnt * 3, frame_cnt * 7, frame_cnt * 11);
    }

    Time_DelayMs(50);
  }

  // 显示平均刷新率
  uint32_t elapsed = Time_GetCurrentMs() - fps_t0;
  if (elapsed == 0) elapsed = 1;
  uint32_t avg_fps = frame_cnt * 1000 / elapsed;
  uint32_t avg_ms = elapsed / frame_cnt;
  OLED_Clear();
  OLED_Printf(8, "FPS: %lu  %lums", avg_fps, avg_ms);
  OLED_Printf(8, "Frames: %lu", frame_cnt);
  OLED_Printf(8, "Time: %lums", elapsed);
  Time_DelayMs(3000);

  // -------------------- 测试3：亮度渐变（对比度 0~255~0） --------------------
  OLED_Clear();
  OLED_Printf(8, "Brightness Test");
  Time_DelayMs(1000);

  // 由暗到亮
  for (int c = 0; c <= 255; c += 5) {
    OLED_SetContrast((uint8_t)c);
    OLED_Clear();
    OLED_Printf(8, "Brightness: %d/255", c);
    OLED_Printf(8, "Rising...");
    Time_DelayMs(30);
  }
  // 由亮到暗
  for (int c = 255; c >= 0; c -= 5) {
    OLED_SetContrast((uint8_t)c);
    OLED_Clear();
    OLED_Printf(8, "Brightness: %d/255", c);
    OLED_Printf(8, "Falling...");
    Time_DelayMs(30);
  }
  OLED_SetContrast(255);  // 恢复最亮
  OLED_Clear();
  OLED_Printf(8, "Brightness: OK!");
  Time_DelayMs(1500);

  // -------------------- 测试4：翻转闪烁 --------------------
  OLED_Clear();
  OLED_Printf(8, "Invert Test");
  Time_DelayMs(1000);

  for (int i = 0; i < 8; i++) {
    OLED_Clear();
    OLED_Invert((i % 2) == 0);
    OLED_Printf(8, "Invert: %s", (i % 2) == 0 ? "ON" : "OFF");
    Time_DelayMs(500);
  }
  OLED_Invert(false);  // 恢复正常
  Time_DelayMs(1000);

  // -------------------- 测试5：全字库展示 --------------------
  OLED_Clear();
  OLED_Printf(8, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%%^&*()_");
  Time_DelayMs(4000);

  // -------------------- 测试6：屏幕开关闪烁 --------------------
  OLED_Clear();
  OLED_Printf(8, "Display ON/OFF");
  Time_DelayMs(1000);

  for (int i = 0; i < 6; i++) {
    OLED_DisplayOn(false);
    Time_DelayMs(300);
    OLED_DisplayOn(true);
    Time_DelayMs(300);
  }
  Time_DelayMs(1000);
  */

  // -------------------- Printf 随机字符串全管线 FPS 测试 --------------------
  {
    #define PRINTF_FPS_CNT 10000

    OLED_Clear();
    OLED_Printf(8, "Printf FPS Test (%d)...", PRINTF_FPS_CNT);
    log_i("FPS", "====== Printf Random FPS Test ======");
    log_i("FPS", "Samples: %d", PRINTF_FPS_CNT);
    OLED_Flush();
    Time_DelayMs(500);

    uint64_t sum = 0;
    uint32_t t_min = 0xFFFFFFFF;
    uint32_t t_max = 0;
    uint64_t fps_acc = 0;
    int fps_cnt = 0;

    // 预分配字符串缓冲区：一行长串，Printf 会自动换行占满屏幕
    // 满屏约 14 字/行 * 7 行 = 98 个字节（size=8, 9px 间距）
    // 再加 FPS 前缀，总共 ~120 字
    static char s_rand_text[140];

    hi_watchdog_disable();

    static char s_fps_str[12] = "..";
    uint32_t rng_seed = 1;

    for (int i = 0; i < PRINTF_FPS_CNT; i++) {
      uint64_t t0 = Time_GetCurrentUs();

      // ========== 现场生成随机可打印 ASCII 字符串 ==========
      // FPS 前缀 + 随机字符填满缓冲区 → Printf 自动换行渲染
      int pos = snprintf(s_rand_text, sizeof(s_rand_text), "FPS:%s ", s_fps_str);
      rng_seed = (uint32_t)(i * 2654435761U);  // 每帧不同种子
      while (pos < (int)(sizeof(s_rand_text) - 5)) {
        rng_seed = rng_seed * 1103515245U + 12345U;
        char c = (char)(' ' + (int)(rng_seed % 95));
        if (c == '%') c = ' ';   // 过滤 %，避免 vsnprintf 解析格式
        s_rand_text[pos++] = c;
      }
      s_rand_text[pos] = '\0';

      // ======== 走完整 Printf 管线 ========
      // Clear + Printf + Flush → 全链路：文本缓冲 → 渲染任务 → I2C 刷新
      OLED_Clear();
      OLED_Printf(8, "%s", s_rand_text);
      OLED_Flush();

      uint64_t dt = Time_GetCurrentUs() - t0;

      fps_acc += dt;
      fps_cnt++;
      if (fps_acc >= 1000000) {
        uint32_t avg_fps_1s = (fps_acc > 0) ? (uint32_t)((uint64_t)fps_cnt * 1000000 / fps_acc) : 0;
        snprintf(s_fps_str, sizeof(s_fps_str), "%lu", avg_fps_1s);
        fps_acc = 0;
        fps_cnt = 0;
      }

      sum += dt;
      if (dt < t_min) t_min = (uint32_t)dt;
      if (dt > t_max) t_max = (uint32_t)dt;
    }

    hi_watchdog_enable();

    uint32_t avg_t = (uint32_t)(sum / PRINTF_FPS_CNT);

    OLED_Clear();
    OLED_Printf(8, "min=%luus  max=%luus", t_min, t_max);
    log_i("FPS", "min=%lu  max=%lu", t_min, t_max);
    OLED_Printf(8, "avg_dt=%luus", avg_t);
    log_i("FPS", "avg_dt=%luus", avg_t);
    OLED_Flush();
    Time_DelayMs(5000);

    OLED_Clear();
    OLED_Printf(8, "1000000/%lu=%lu", avg_t, 1000000 / avg_t);
    log_i("FPS", "1000000 / %lu = %lu (Avg FPS)", avg_t, 1000000 / avg_t);
    OLED_Printf(8, "minFPS=1000000/%lu=%lu", t_max, 1000000 / t_max);
    log_i("FPS", "1000000 / %lu = %lu (Min FPS)", t_max, 1000000 / t_max);
    OLED_Printf(8, "maxFPS=1000000/%lu=%lu", t_min, 1000000 / t_min);
    log_i("FPS", "1000000 / %lu = %lu (Max FPS)", t_min, 1000000 / t_min);
    OLED_Flush();
    Time_DelayMs(10000);
  }

  // -------------------- 测试完成 --------------------
  OLED_Clear();
  OLED_Printf(8, "All Tests Done!");
  OLED_Printf(8, "OLED OK!");
  log_i("FPS", "====== Printf Random FPS Test Done ======");
  Time_DelayMs(3000);
  // ========== OLED 屏幕测试结束 ==========

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

  // 初始化报警系统
  log_i("ALARM_SYSTEM", "正在初始化报警系统...");

  if (alarm_system_init(NULL)) {
    log_i("ALARM_SYSTEM", "✅ 报警系统初始化成功");
    // 设置报警回调函数
    alarm_system_set_callback(alarm_event_handler);
    log_i("ALARM_SYSTEM", "✅ 报警回调函数设置完成");
  } else {
    log_e("ALARM_SYSTEM", "❌ 报警系统初始化失败");
  }

  log_i("SYSTEM", "系统初始化完成");
  // 创建主任务
  log_i("SYSTEM", "正在创建主任务...");
  osThreadAttr_t main_task_attr = {.name = "MainTask",
                                   .attr_bits = 0U,
                                   .cb_mem = NULL,
                                   .cb_size = 0U,
                                   .stack_mem = NULL,
                                   .stack_size = 8192,
                                   .priority = osPriorityNormal};

  if (osThreadNew(Main_Task, NULL, &main_task_attr) == NULL) {
    log_e("SYSTEM", "主任务创建失败!");
  } else {
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
      .stack_size = 8192,  // 增加栈大小以容纳合并后的功能
      .priority = osPriorityNormal};

  if (osThreadNew(network_task, NULL, &network_attr) == NULL) {
    log_e("NETWORK", "网络任务创建失败!");
  } else {
    log_i("NETWORK", "网络任务创建成功");
  }
}

// 应用入口函数
SYS_RUN(Main_Entry);