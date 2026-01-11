#include "alarm_system.h"
#include <string.h>

// 报警系统全局状态
static alarm_config_t g_alarm_config = {0};
static alarm_status_t g_alarm_status = {0};
static alarm_callback_t g_alarm_callback = NULL;

// 报警级别字符串
static const char* alarm_level_strings[] = {
    "NONE",
    "YELLOW",
    "RED"
};

// 报警类型字符串
static const char* alarm_type_strings[] = {
    "TEMPERATURE",
    "HUMIDITY",
    "SMOKE"
};

bool alarm_system_init(const alarm_config_t* config)
{
    if (config == NULL) {
        return false;
    }
    
    // 复制配置
    memcpy(&g_alarm_config, config, sizeof(alarm_config_t));
    
    // 初始化状态
    memset(&g_alarm_status, 0, sizeof(alarm_status_t));
    g_alarm_status.temp_alarm_level = ALARM_LEVEL_NONE;
    g_alarm_status.hum_alarm_level = ALARM_LEVEL_NONE;
    g_alarm_status.smoke_alarm_level = ALARM_LEVEL_NONE;
    
    return true;
}

void alarm_system_deinit(void)
{
    memset(&g_alarm_config, 0, sizeof(alarm_config_t));
    memset(&g_alarm_status, 0, sizeof(alarm_status_t));
    g_alarm_callback = NULL;
}

bool alarm_system_update(float temperature, float humidity, float smoke)
{
    bool has_alarm = false;
    bool has_clear_alarm = false;
    alarm_event_t event = {0};
    
    // 检测温度报警
    if (temperature >= g_alarm_config.temp_red_threshold) {
        if (g_alarm_status.temp_alarm_level != ALARM_LEVEL_RED) {
            g_alarm_status.temp_alarm_level = ALARM_LEVEL_RED;
            event.type = ALARM_TYPE_TEMPERATURE;
            event.level = ALARM_LEVEL_RED;
            event.value = temperature;
            event.threshold = g_alarm_config.temp_red_threshold;
            has_alarm = true;
        }
    } else if (temperature >= g_alarm_config.temp_yellow_threshold) {
        if (g_alarm_status.temp_alarm_level != ALARM_LEVEL_YELLOW) {
            g_alarm_status.temp_alarm_level = ALARM_LEVEL_YELLOW;
            event.type = ALARM_TYPE_TEMPERATURE;
            event.level = ALARM_LEVEL_YELLOW;
            event.value = temperature;
            event.threshold = g_alarm_config.temp_yellow_threshold;
            has_alarm = true;
        }
    } else {
        if (g_alarm_status.temp_alarm_level != ALARM_LEVEL_NONE) {
            g_alarm_status.temp_alarm_level = ALARM_LEVEL_NONE;
            event.type = ALARM_TYPE_TEMPERATURE;
            event.level = ALARM_LEVEL_NONE;
            event.value = temperature;
            event.threshold = 0;
            has_clear_alarm = true;
        }
    }
    
    // 检测湿度报警
    if (humidity >= g_alarm_config.hum_red_threshold) {
        if (g_alarm_status.hum_alarm_level != ALARM_LEVEL_RED) {
            g_alarm_status.hum_alarm_level = ALARM_LEVEL_RED;
            event.type = ALARM_TYPE_HUMIDITY;
            event.level = ALARM_LEVEL_RED;
            event.value = humidity;
            event.threshold = g_alarm_config.hum_red_threshold;
            has_alarm = true;
        }
    } else if (humidity >= g_alarm_config.hum_yellow_threshold) {
        if (g_alarm_status.hum_alarm_level != ALARM_LEVEL_YELLOW) {
            g_alarm_status.hum_alarm_level = ALARM_LEVEL_YELLOW;
            event.type = ALARM_TYPE_HUMIDITY;
            event.level = ALARM_LEVEL_YELLOW;
            event.value = humidity;
            event.threshold = g_alarm_config.hum_yellow_threshold;
            has_alarm = true;
        }
    } else {
        if (g_alarm_status.hum_alarm_level != ALARM_LEVEL_NONE) {
            g_alarm_status.hum_alarm_level = ALARM_LEVEL_NONE;
            event.type = ALARM_TYPE_HUMIDITY;
            event.level = ALARM_LEVEL_NONE;
            event.value = humidity;
            event.threshold = 0;
            has_clear_alarm = true;
        }
    }
    
    // 检测烟雾报警
    if (smoke >= g_alarm_config.smoke_red_threshold) {
        if (g_alarm_status.smoke_alarm_level != ALARM_LEVEL_RED) {
            g_alarm_status.smoke_alarm_level = ALARM_LEVEL_RED;
            event.type = ALARM_TYPE_SMOKE;
            event.level = ALARM_LEVEL_RED;
            event.value = smoke;
            event.threshold = g_alarm_config.smoke_red_threshold;
            has_alarm = true;
        }
    } else if (smoke >= g_alarm_config.smoke_yellow_threshold) {
        if (g_alarm_status.smoke_alarm_level != ALARM_LEVEL_YELLOW) {
            g_alarm_status.smoke_alarm_level = ALARM_LEVEL_YELLOW;
            event.type = ALARM_TYPE_SMOKE;
            event.level = ALARM_LEVEL_YELLOW;
            event.value = smoke;
            event.threshold = g_alarm_config.smoke_yellow_threshold;
            has_alarm = true;
        }
    } else {
        if (g_alarm_status.smoke_alarm_level != ALARM_LEVEL_NONE) {
            g_alarm_status.smoke_alarm_level = ALARM_LEVEL_NONE;
            event.type = ALARM_TYPE_SMOKE;
            event.level = ALARM_LEVEL_NONE;
            event.value = smoke;
            event.threshold = 0;
            has_clear_alarm = true;
        }
    }
    
    // 如果有报警事件，调用回调函数
    if ((has_alarm || has_clear_alarm) && g_alarm_callback != NULL) {
        g_alarm_callback(&event);
    }
    
    return has_alarm;
}

void alarm_system_set_callback(alarm_callback_t callback)
{
    g_alarm_callback = callback;
}

void alarm_system_get_status(alarm_status_t* status)
{
    if (status != NULL) {
        memcpy(status, &g_alarm_status, sizeof(alarm_status_t));
    }
}

bool alarm_system_trigger_alarm(alarm_type_t type, alarm_level_t level)
{
    alarm_event_t event = {0};
    event.type = type;
    event.level = level;
    
    // 更新内部状态
    switch (type) {
        case ALARM_TYPE_TEMPERATURE:
            g_alarm_status.temp_alarm_level = level;
            break;
        case ALARM_TYPE_HUMIDITY:
            g_alarm_status.hum_alarm_level = level;
            break;
        case ALARM_TYPE_SMOKE:
            g_alarm_status.smoke_alarm_level = level;
            break;
        default:
            return false;
    }
    
    // 调用回调函数
    if (g_alarm_callback != NULL) {
        g_alarm_callback(&event);
        return true;
    }
    
    return false;
}

bool alarm_system_clear_alarm(alarm_type_t type)
{
    switch (type) {
        case ALARM_TYPE_TEMPERATURE:
            g_alarm_status.temp_alarm_level = ALARM_LEVEL_NONE;
            break;
        case ALARM_TYPE_HUMIDITY:
            g_alarm_status.hum_alarm_level = ALARM_LEVEL_NONE;
            break;
        case ALARM_TYPE_SMOKE:
            g_alarm_status.smoke_alarm_level = ALARM_LEVEL_NONE;
            break;
        default:
            return false;
    }
    return true;
}

const char* alarm_system_get_level_string(alarm_level_t level)
{
    if (level >= ALARM_LEVEL_NONE && level <= ALARM_LEVEL_RED) {
        return alarm_level_strings[level];
    }
    return "UNKNOWN";
}

const char* alarm_system_get_type_string(alarm_type_t type)
{
    if (type >= ALARM_TYPE_TEMPERATURE && type <= ALARM_TYPE_SMOKE) {
        return alarm_type_strings[type];
    }
    return "UNKNOWN";
}