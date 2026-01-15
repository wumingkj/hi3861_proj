/**
 ****************************************************************************************************
 * @file        bsp_wifi.h
 * @author      普中科技
 * @version     V1.0
 * @date        2024-06-05
 * @brief       WIFI AP实验
 * @license     Copyright (c) 2024-2034, 深圳市普中科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:普中-Hi3861
 * 在线视频:https://space.bilibili.com/2146492485
 * 公司网址:www.prechin.cn
 * 购买地址:
 *
 */

#ifndef WIFI_H
#define WIFI_H

#include "cmsis_os2.h"
#include "hi_io.h"
#include "hi_gpio.h"
#include "wifi_error_code.h"
#include "wifi_device.h"
#include "wifi_hotspot.h"
#include "lwip/netifapi.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/api_shell.h"


#include "debug.h"
// 使用系统定义的常量，避免重复定义
#ifndef WIFI_MAX_SSID_LEN
#define WIFI_MAX_SSID_LEN 33  // 使用系统定义的值
#endif

#ifndef WIFI_MAX_KEY_LEN  
#define WIFI_MAX_KEY_LEN 65   // 使用系统定义的值
#endif

//函数声明
WifiErrorCode WiFi_createHotspots(const char *ssid, const char *psk);
WifiErrorCode WiFi_connectHotspots(const char *ssid, const char *psk);
char* WiFi_GetLocalIP(void);

// 新增：设置AP的IP配置
WifiErrorCode WiFi_SetAPConfig(const char *ip_addr, const char *netmask, const char *gateway);

// 新增：STA相关函数
WifiErrorCode WiFi_setSTAInfo(const char *ssid, const char *password, int security_type);
WifiErrorCode WiFi_connectToSTA(void);
WifiErrorCode WiFi_startAPAndConnectSTA(const char *ap_ssid, const char *ap_password, 
                                       const char *sta_ssid, const char *sta_password);


// 新增：WiFi连接状态枚举
typedef enum {
  WIFI_STATE_IDLE = 0,
  WIFI_STATE_CONNECTING,
  WIFI_STATE_WAITING_DHCP,
  WIFI_STATE_CONNECTED,
  WIFI_STATE_FAILED
} wifi_connect_state_t;

// 新增：非阻塞WiFi连接函数
WifiErrorCode WiFi_connectHotspotsAsync(const char *ssid, const char *psk);
WifiErrorCode WiFi_processConnection(void);
wifi_connect_state_t WiFi_getConnectionState(void);
bool WiFi_isConnecting(void);

#endif