#include "alarm_system.h"

#include <string.h>

#include "kv.h"
// 报警系统全局状态
static alarm_config_t g_alarm_config = {0};
static alarm_status_t g_alarm_status = {0};
static alarm_callback_t g_alarm_callback = NULL;

// 报警级别字符串
static const char* alarm_level_strings[] = {"NONE", "YELLOW", "RED"};

// 报警类型字符串
static const char* alarm_type_strings[] = {"TEMPERATURE", "HUMIDITY", "SMOKE"};

// 默认报警配置
const alarm_config_t DEFAULT_ALARM_CONFIG = {
    .temp_yellow_threshold = 25,     // 温度黄色报警阈值
    .temp_red_threshold = 30,        // 温度红色报警阈值
    .hum_yellow_threshold = 50,      // 湿度黄色报警阈值
    .hum_red_threshold = 60,         // 湿度红色报警阈值
    .smoke_yellow_threshold = 1000,  // 烟雾黄色报警阈值
    .smoke_red_threshold = 1500      // 烟雾红色报警阈值
};

bool alarm_system_init(const alarm_config_t* config) {
    alarm_config_t temp_config;
    
    if (config != NULL) {
        // 如果传入了配置参数，直接使用
        memcpy(&temp_config, config, sizeof(alarm_config_t));
        printf("Alarm system initialized with provided config\n");
    } else {
        // 如果配置为NULL，尝试从KV加载
        if (alarm_system_load_config_from_kv(&temp_config)) {
            printf("Alarm config loaded from KV storage\n");
        } else {
            // KV加载失败，使用默认配置
            printf("Warning: Failed to load alarm config from KV, using default config\n");
            memcpy(&temp_config, &DEFAULT_ALARM_CONFIG, sizeof(alarm_config_t));
            
            // 将默认配置写入KV库
            kv_set_uint32("alarm_temp_yellow", temp_config.temp_yellow_threshold);
            kv_set_uint32("alarm_temp_red", temp_config.temp_red_threshold);
            kv_set_uint32("alarm_hum_yellow", temp_config.hum_yellow_threshold);
            kv_set_uint32("alarm_hum_red", temp_config.hum_red_threshold);
            kv_set_uint32("alarm_smoke_yellow", temp_config.smoke_yellow_threshold);
            kv_set_uint32("alarm_smoke_red", temp_config.smoke_red_threshold);
            
            printf("Default alarm config saved to KV storage\n");
        }
    }
    
    // 复制配置到全局变量
    memcpy(&g_alarm_config, &temp_config, sizeof(alarm_config_t));
    
    // 初始化状态
    memset(&g_alarm_status, 0, sizeof(alarm_status_t));
    g_alarm_status.temp_alarm_level = ALARM_LEVEL_NONE;
    g_alarm_status.hum_alarm_level = ALARM_LEVEL_NONE;
    g_alarm_status.smoke_alarm_level = ALARM_LEVEL_NONE;
    
    printf("Alarm system initialized: T(Y:%u,R:%u) H(Y:%u,R:%u) S(Y:%u,R:%u)\n",
           g_alarm_config.temp_yellow_threshold, g_alarm_config.temp_red_threshold,
           g_alarm_config.hum_yellow_threshold, g_alarm_config.hum_red_threshold,
           g_alarm_config.smoke_yellow_threshold, g_alarm_config.smoke_red_threshold);

    return true;
}
bool alarm_system_update(uint32_t temperature, uint32_t humidity, uint32_t smoke) {
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
            event.threshold = g_alarm_config.temp_red_threshold; // 使用全局配置中的阈值
            has_alarm = true;
        }
    } else if (temperature >= g_alarm_config.temp_yellow_threshold) {
        if (g_alarm_status.temp_alarm_level != ALARM_LEVEL_YELLOW) {
            g_alarm_status.temp_alarm_level = ALARM_LEVEL_YELLOW;
            event.type = ALARM_TYPE_TEMPERATURE;
            event.level = ALARM_LEVEL_YELLOW;
            event.value = temperature;
            event.threshold = g_alarm_config.temp_yellow_threshold; // 使用全局配置中的阈值
            has_alarm = true;
        }
    } else {
        if (g_alarm_status.temp_alarm_level != ALARM_LEVEL_NONE) {
            g_alarm_status.temp_alarm_level = ALARM_LEVEL_NONE;
            event.type = ALARM_TYPE_TEMPERATURE;
            event.level = ALARM_LEVEL_NONE;
            event.value = temperature;
            event.threshold = g_alarm_config.temp_red_threshold; // 使用全局配置中的阈值，而不是0
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
            event.threshold = g_alarm_config.hum_red_threshold; // 使用全局配置中的阈值
            has_alarm = true;
        }
    } else if (humidity >= g_alarm_config.hum_yellow_threshold) {
        if (g_alarm_status.hum_alarm_level != ALARM_LEVEL_YELLOW) {
            g_alarm_status.hum_alarm_level = ALARM_LEVEL_YELLOW;
            event.type = ALARM_TYPE_HUMIDITY;
            event.level = ALARM_LEVEL_YELLOW;
            event.value = humidity;
            event.threshold = g_alarm_config.hum_yellow_threshold; // 使用全局配置中的阈值
            has_alarm = true;
        }
    } else {
        if (g_alarm_status.hum_alarm_level != ALARM_LEVEL_NONE) {
            g_alarm_status.hum_alarm_level = ALARM_LEVEL_NONE;
            event.type = ALARM_TYPE_HUMIDITY;
            event.level = ALARM_LEVEL_NONE;
            event.value = humidity;
            event.threshold = g_alarm_config.hum_red_threshold; // 使用全局配置中的阈值，而不是0
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
            event.threshold = g_alarm_config.smoke_red_threshold; // 使用全局配置中的阈值
            has_alarm = true;
        }
    } else if (smoke >= g_alarm_config.smoke_yellow_threshold) {
        if (g_alarm_status.smoke_alarm_level != ALARM_LEVEL_YELLOW) {
            g_alarm_status.smoke_alarm_level = ALARM_LEVEL_YELLOW;
            event.type = ALARM_TYPE_SMOKE;
            event.level = ALARM_LEVEL_YELLOW;
            event.value = smoke;
            event.threshold = g_alarm_config.smoke_yellow_threshold; // 使用全局配置中的阈值
            has_alarm = true;
        }
    } else {
        if (g_alarm_status.smoke_alarm_level != ALARM_LEVEL_NONE) {
            g_alarm_status.smoke_alarm_level = ALARM_LEVEL_NONE;
            event.type = ALARM_TYPE_SMOKE;
            event.level = ALARM_LEVEL_NONE;
            event.value = smoke;
            event.threshold = g_alarm_config.smoke_red_threshold; // 使用全局配置中的阈值，而不是0
            has_clear_alarm = true;
        }
    }

    // 如果有报警事件，调用回调函数
    if ((has_alarm || has_clear_alarm) && g_alarm_callback != NULL) {
        g_alarm_callback(&event);
    }

    return has_alarm;
}

void alarm_system_set_callback(alarm_callback_t callback) {
    g_alarm_callback = callback;
}

void alarm_system_get_status(alarm_status_t* status) {
    if (status != NULL) {
        memcpy(status, &g_alarm_status, sizeof(alarm_status_t));
    }
}

bool alarm_system_trigger_alarm(alarm_type_t type, alarm_level_t level) {
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

bool alarm_system_clear_alarm(alarm_type_t type) {
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

const char* alarm_system_get_level_string(alarm_level_t level) {
    if (level >= ALARM_LEVEL_NONE && level <= ALARM_LEVEL_RED) {
        return alarm_level_strings[level];
    }
    return "UNKNOWN";
}

const char* alarm_system_get_type_string(alarm_type_t type) {
    if (type >= ALARM_TYPE_TEMPERATURE && type <= ALARM_TYPE_SMOKE) {
        return alarm_type_strings[type];
    }
    return "UNKNOWN";
}

static bool alarm_system_load_config_from_kv(alarm_config_t* config) {
    bool success = true;

    // 尝试从KV读取所有阈值
    if (kv_get_uint32("alarm_temp_yellow", &config->temp_yellow_threshold) != KV_SUCCESS) {
        success = false;
    }
    if (kv_get_uint32("alarm_temp_red", &config->temp_red_threshold) != KV_SUCCESS) {
        success = false;
    }
    if (kv_get_uint32("alarm_hum_yellow", &config->hum_yellow_threshold) != KV_SUCCESS) {
        success = false;
    }
    if (kv_get_uint32("alarm_hum_red", &config->hum_red_threshold) != KV_SUCCESS) {
        success = false;
    }
    if (kv_get_uint32("alarm_smoke_yellow", &config->smoke_yellow_threshold) != KV_SUCCESS) {
        success = false;
    }
    if (kv_get_uint32("alarm_smoke_red", &config->smoke_red_threshold) != KV_SUCCESS) {
        success = false;
    }

    return success;
}

// 获取当前配置的函数
void alarm_system_get_config(alarm_config_t* config) {
    if (config != NULL) {
        memcpy(config, &g_alarm_config, sizeof(alarm_config_t));
    }
}