/**
 ****************************************************************************************************
 * @file        bsp_wifi.c
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
#include "wifi.h"
#include <unistd.h>
#include <string.h>

#define DEF_TIMEOUT 15
static int g_ConnectSuccess = 0;
#define SELECT_WLAN_PORT "wlan0"
static struct netif *g_lwip_netif = NULL;
static char g_IP_Addr[20] = {"192.168.0.1"}; // 默认AP IP地址改为192.168.0.1

// AP配置结构体
typedef struct {
    char ip_addr[20];
    char netmask[20];
    char gateway[20];
} ap_config_t;

static ap_config_t g_ap_config = {
    .ip_addr = "192.168.0.1",
    .netmask = "255.255.255.0",
    .gateway = "192.168.0.1"
};

// STA信息结构体
typedef struct {
    char ssid[WIFI_MAX_SSID_LEN+1];
    char password[WIFI_MAX_KEY_LEN+1];
    int security_type;
} sta_info_t;

static sta_info_t g_sta_info = {0};
static int g_sta_info_set = 0;

/**
 * @brief  获取连接WiFi后的本地IP地址
 * @note
 * @retval IP地址-字符串
 */
char *WiFi_GetLocalIP(void)
{
    return g_IP_Addr;
}

/**
 * 设置AP的IP配置
 **/
WifiErrorCode WiFi_SetAPConfig(const char *ip_addr, const char *netmask, const char *gateway)
{
    if (ip_addr) {
        strcpy_s(g_ap_config.ip_addr, sizeof(g_ap_config.ip_addr), ip_addr);
        strcpy_s(g_IP_Addr, sizeof(g_IP_Addr), ip_addr);
    }
    if (netmask) {
        strcpy_s(g_ap_config.netmask, sizeof(g_ap_config.netmask), netmask);
    }
    if (gateway) {
        strcpy_s(g_ap_config.gateway, sizeof(g_ap_config.gateway), gateway);
    }
    
    log_i("WIFI", "AP配置已设置: IP=%s, 掩码=%s, 网关=%s", 
          g_ap_config.ip_addr, g_ap_config.netmask, g_ap_config.gateway);
    return WIFI_SUCCESS;
}

/**
 * 获取WiFi的IP地址
 **/
static void Sta_GetWiFiIP(struct netif *netif, char *ip)
{
    int ret;
    if (netif == NULL) {
        return;
    }

    ip4_addr_t ipAddr;
    ip4_addr_t netMask;
    ip4_addr_t gateWay;

    ret = netifapi_netif_get_addr(netif, &ipAddr, &netMask, &gateWay);
    if (ret == 0) {
        inet_ntop(AF_INET, &ipAddr, ip, INET_ADDRSTRLEN);
    }
}

#define WIFI_CHANNE 5 // WiFi通道

WifiErrorCode WiFi_createHotspots(const char *ssid, const char *psk)
{
    WifiErrorCode result;
    log_i("WIFI", "开始初始化WiFi热点");

    // 使能WiFi
    result = EnableWifi();
    if (result != WIFI_SUCCESS) {
        log_e("WIFI", "WiFi使能失败. 错误码: %d", result);
        return result;
    }
    // 判断WiFi是否激活
    result = IsWifiActive();
    if (result != WIFI_STA_ACTIVE) {
        log_e("WIFI", "WiFi激活失败. 错误码: %d", result);
        return result;
    }

    // 设置指定的热点信息
    HotspotConfig hotspotConfig = {0};
    strcpy_s(hotspotConfig.ssid, strlen(ssid) + 1, ssid);
    strcpy_s(hotspotConfig.preSharedKey, strlen(psk) + 1, psk);
    hotspotConfig.securityType = WIFI_SEC_TYPE_PSK;
    hotspotConfig.band = HOTSPOT_BAND_TYPE_2G;
    hotspotConfig.channelNum = WIFI_CHANNE;
    result = SetHotspotConfig(&hotspotConfig);
    if (result != WIFI_SUCCESS) {
        log_e("WIFI", "设置WiFi热点信息失败. 错误码: %d", result);
        return result;
    }

    // 开启WiFi热点模式
    result = EnableHotspot();
    if (result != WIFI_SUCCESS) {
        log_e("WIFI", "开启WiFi热点模式失败. 错误码: %d", result);
        return result;
    }

    // 检查WiFi热点是否激活
    result = IsHotspotActive();
    if (result != WIFI_HOTSPOT_ACTIVE) {
        log_e("WIFI", "WiFi热点激活失败. 错误码: %d", result);
        return result;
    }

    // 设置AP的静态IP地址
    g_lwip_netif = netifapi_netif_find(SELECT_WLAN_PORT);
    if (g_lwip_netif) {
        ip4_addr_t ip_addr, netmask, gateway;
        
        // 转换IP地址字符串为ip4_addr_t
        inet_pton(AF_INET, g_ap_config.ip_addr, &ip_addr);
        inet_pton(AF_INET, g_ap_config.netmask, &netmask);
        inet_pton(AF_INET, g_ap_config.gateway, &gateway);
        
        // 设置静态IP
        netifapi_netif_set_addr(g_lwip_netif, &ip_addr, &netmask, &gateway);
        log_i("WIFI", "AP静态IP设置成功: %s", g_ap_config.ip_addr);
    }

    log_i("WIFI", "WiFi热点初始化成功");
    log_i("WIFI", "热点信息: SSID=%s, IP=%s", ssid, g_ap_config.ip_addr);

    return WIFI_SUCCESS;
}

// 连接WiFi热点时的状态发生改变的回调函数
static void ConnectionWifiChangedHandler(int state, WifiLinkedInfo *info)
{
    if (info == NULL) {
        log_w("WIFI", "WifiConnectionChanged: 信息为空");
    } else {
        if (state == WIFI_STATE_AVALIABLE) {
            g_ConnectSuccess = 1;
            log_i("WIFI", "WiFi连接可用");
        } else {
            g_ConnectSuccess = 0;
            log_w("WIFI", "WiFi连接丢失");
        }
    }
}

// 等待连接热点 默认15s的超时时间
static int WaitConnectResult(void)
{
    int ConnectTimeout = DEF_TIMEOUT;
    while (ConnectTimeout > 0) {
        sleep(1);
        ConnectTimeout--;
        log_d("WIFI", "等待连接中...");
        if (g_ConnectSuccess == 1) {
            log_i("WIFI", "等待连接成功[%d]秒", (DEF_TIMEOUT - ConnectTimeout));
            break;
        }
    }
    if (ConnectTimeout <= 0) {
        log_e("WIFI", "等待连接超时!");
        return 0;
    }

    return 1;
}

WifiErrorCode WiFi_connectHotspots(const char *ssid, const char *psk)
{
    WifiErrorCode result;
    int Timeout = 10; // 超时时间 10s

    log_i("WIFI", "开始连接WiFi热点");

    // 使能WiFi
    result = EnableWifi();
    if (result != WIFI_SUCCESS) {
        log_e("WIFI", "WiFi使能失败");
        return result;
    }
    // 判断WiFi是否激活
    result = IsWifiActive();
    if (result != WIFI_STA_ACTIVE) {
        log_e("WIFI", "WiFi激活失败");
        return result;
    }
    // 注册wifi的回调函数
    WifiEvent eventConfig = {0};
    eventConfig.OnWifiConnectionChanged = ConnectionWifiChangedHandler; // WiFi连接的状态改变
    result = RegisterWifiEvent(&eventConfig);
    if (result != WIFI_SUCCESS) {
        log_e("WIFI", "注册WiFi回调函数失败");
        return result;
    }

    // 连接指定的WiFi热点
    WifiDeviceConfig wifiDeviceConfig = {0};
    int wifiResult = 0;
    strcpy_s(wifiDeviceConfig.ssid, strlen(ssid) + 1, ssid);               // 连接WiFi的名称
    strcpy_s(wifiDeviceConfig.preSharedKey, strlen(psk) + 1, psk);        // WiFi的密码
    wifiDeviceConfig.securityType = WIFI_SEC_TYPE_PSK; // WiFi的安全性
    result = AddDeviceConfig(&wifiDeviceConfig, &wifiResult);
     if ((result == WIFI_SUCCESS) && (ConnectTo(wifiResult) == WIFI_SUCCESS)&& (WaitConnectResult() == 1)) {
        log_i("WIFI", "WiFi连接成功!");
        g_lwip_netif = netifapi_netif_find(SELECT_WLAN_PORT);
        //启动DHCP
        if (g_lwip_netif) {
            dhcp_start(g_lwip_netif);
        }

        //等待DHCP
        for (;;) {
            if (dhcp_is_bound(g_lwip_netif) == ERR_OK) {
                Sta_GetWiFiIP(g_lwip_netif, g_IP_Addr);
                log_i("WIFI", "连接WiFi获取到IP地址: %s", g_IP_Addr);
                break;
            }
            log_d("WIFI", "等待DHCP分配...");
            Timeout--;
            if (Timeout == 0) {
                // 超时
                log_e("WIFI", "DHCP超时");
                return ERROR_WIFI_BUSY;
            }
            sleep(1);
        }
    } else {
        log_e("WIFI", "WiFi连接失败");
        return ERROR_WIFI_BUSY;
     }
    return WIFI_SUCCESS;
}

// 新增：设置STA信息
WifiErrorCode WiFi_setSTAInfo(const char *ssid, const char *password, int security_type)
{
    if (ssid == NULL || password == NULL) {
        log_e("WIFI", "STA info is invalid");
        return ERROR_WIFI_INVALID_ARGS;
    }
    
    strcpy_s(g_sta_info.ssid, sizeof(g_sta_info.ssid), ssid);
    strcpy_s(g_sta_info.password, sizeof(g_sta_info.password), password);
    g_sta_info.security_type = security_type;
    g_sta_info_set = 1;
    
    log_i("WIFI", "STA info set successfully: SSID=%s, Security=%d", ssid, security_type);
    return WIFI_SUCCESS;
}

// 新增：连接到预先设置的STA
WifiErrorCode WiFi_connectToSTA(void)
{
    if (!g_sta_info_set) {
        log_e("WIFI", "STA info not set, please call WiFi_setSTAInfo first");
        return ERROR_WIFI_INVALID_ARGS;
    }
    
    log_i("WIFI", "Connecting to STA: %s", g_sta_info.ssid);
    return WiFi_connectHotspots(g_sta_info.ssid, g_sta_info.password);
}

// 新增：完整的AP+STA流程
WifiErrorCode WiFi_startAPAndConnectSTA(const char *ap_ssid, const char *ap_password, 
                                       const char *sta_ssid, const char *sta_password)
{
    WifiErrorCode result;
    
    // 步骤1：创建AP
    log_i("WIFI", "=== Step 1: Creating WiFi AP ===");
    result = WiFi_createHotspots(ap_ssid, ap_password);
    if (result != WIFI_SUCCESS) {
        log_e("WIFI", "Failed to create AP");
        return result;
    }
    
    // 模拟连接到服务端并获取STA信息的过程
    log_i("WIFI", "=== Step 2: Simulating connection to server to get STA info ===");
    log_i("WIFI", "Connected to server, receiving STA configuration...");
    
    // 步骤3：设置STA信息
    log_i("WIFI", "=== Step 3: Setting STA information ===");
    result = WiFi_setSTAInfo(sta_ssid, sta_password, WIFI_SEC_TYPE_PSK);
    if (result != WIFI_SUCCESS) {
        log_e("WIFI", "Failed to set STA info");
        return result;
    }
    
    // 步骤4：连接到STA
    log_i("WIFI", "=== Step 4: Connecting to STA ===");
    result = WiFi_connectToSTA();
    if (result != WIFI_SUCCESS) {
        log_e("WIFI", "Failed to connect to STA");
        return result;
    }
    
    log_i("WIFI", "=== AP+STA process completed successfully ===");
    return WIFI_SUCCESS;
}