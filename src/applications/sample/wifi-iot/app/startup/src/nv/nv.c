/*
 * Copyright (c) 2024 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "nv.h"
#include <string.h>
#include <hi_nv.h>

// 内部数据结构
typedef struct {
    nv_kv_pair_t kv_pairs[NV_MAX_KV_PAIRS];    // KV对数组
    uint16_t kv_count;                         // 当前KV对数量
    nv_config_t config;                        // 配置信息
    bool initialized;                          // 初始化标志
    uint32_t last_save_time;                   // 最后保存时间
    uint32_t used_size;                        // 已使用大小
} nv_context_t;

static nv_context_t g_nv_ctx = {0};

// CRC16计算函数
static uint16_t nv_crc16(const uint8_t* data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    
    if (data == NULL || len == 0) {
        return crc;
    }
    
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    
    return crc;
}

// 查找KV对的索引
static int16_t nv_find_key_index(const char* key) {
    if (key == NULL || strlen(key) == 0) {
        return -1;
    }
    
    for (uint16_t i = 0; i < g_nv_ctx.kv_count; i++) {
        if (strcmp(g_nv_ctx.kv_pairs[i].key, key) == 0) {
            return i;
        }
    }
    
    return -1;
}

// 计算KV对占用的存储大小
static uint32_t nv_calc_kv_size(const nv_kv_pair_t* kv) {
    if (kv == NULL) {
        return 0;
    }
    
    return sizeof(nv_kv_pair_t) - NV_MAX_VALUE_LEN + kv->value_len;
}

nv_result_t nv_init(const nv_config_t* config) {
    if (config == NULL) {
        log_e("[NV]", "Invalid config parameter");
        return NV_ERROR_INVALID_PARAM;
    }
    
    if (g_nv_ctx.initialized) {
        log_w("[NV]", "NV library already initialized");
        return NV_SUCCESS;
    }
    
    // 初始化配置
    memcpy(&g_nv_ctx.config, config, sizeof(nv_config_t));
    
    // 初始化非工厂NV
    hi_u32 ret = hi_nv_init(config->base_addr, config->total_size, config->block_size);
    if (ret != HI_ERR_SUCCESS) {
        log_e("[NV]", "Failed to initialize NV: 0x%X", ret);
        return NV_ERROR_INIT_FAILED;
    }
    
    // 从Flash加载数据
    nv_result_t load_ret = nv_load_from_flash();
    if (load_ret != NV_SUCCESS) {
        log_w("[NV]", "Failed to load data from flash, using empty storage");
        g_nv_ctx.kv_count = 0;
        g_nv_ctx.used_size = 0;
    }
    
    g_nv_ctx.initialized = true;
    g_nv_ctx.last_save_time = 0;
    
    log_i("[NV]", "NV library initialized successfully");
    log_i("[NV]", "Version: %s, Max KV pairs: %d", NV_LIB_VERSION, NV_MAX_KV_PAIRS);
    log_i("[NV]", "Base addr: 0x%08X, Total size: %d bytes", config->base_addr, config->total_size);
    
    return NV_SUCCESS;
}

void nv_deinit(void) {
    if (!g_nv_ctx.initialized) {
        return;
    }
    
    // 保存数据到Flash
    nv_save_to_flash();
    
    memset(&g_nv_ctx, 0, sizeof(nv_context_t));
    log_i("[NV]", "NV library deinitialized");
}

nv_result_t nv_set(const char* key, const void* value, uint16_t value_len) {
    if (!g_nv_ctx.initialized) {
        log_e("[NV]", "NV library not initialized");
        return NV_ERROR_NOT_INITIALIZED;
    }
    
    if (key == NULL || value == NULL) {
        log_e("[NV]", "Invalid key or value parameter");
        return NV_ERROR_INVALID_PARAM;
    }
    
    uint16_t key_len = strlen(key);
    if (key_len == 0 || key_len >= NV_MAX_KEY_LEN) {
        log_e("[NV]", "Key too long or empty: %s", key);
        return NV_ERROR_KEY_TOO_LONG;
    }
    
    if (value_len == 0 || value_len > NV_MAX_VALUE_LEN) {
        log_e("[NV]", "Value length invalid: %d", value_len);
        return NV_ERROR_VALUE_TOO_LONG;
    }
    
    int16_t index = nv_find_key_index(key);
    nv_kv_pair_t* kv = NULL;
    
    if (index >= 0) {
        // 更新现有KV对
        kv = &g_nv_ctx.kv_pairs[index];
        g_nv_ctx.used_size -= nv_calc_kv_size(kv);
        log_d("[NV]", "Updating existing key: %s", key);
    } else {
        // 创建新KV对
        if (g_nv_ctx.kv_count >= NV_MAX_KV_PAIRS) {
            log_e("[NV]", "Storage full, cannot add new key: %s", key);
            return NV_ERROR_STORAGE_FULL;
        }
        
        index = g_nv_ctx.kv_count;
        kv = &g_nv_ctx.kv_pairs[index];
        g_nv_ctx.kv_count++;
        log_d("[NV]", "Adding new key: %s", key);
    }
    
    // 设置KV对数据
    strncpy(kv->key, key, NV_MAX_KEY_LEN - 1);
    kv->key[NV_MAX_KEY_LEN - 1] = '\0';
    memcpy(kv->value, value, value_len);
    kv->value_len = value_len;
    kv->timestamp = Time_GetCurrentMs();
    kv->crc = nv_crc16((uint8_t*)value, value_len);
    
    // 更新已使用大小
    g_nv_ctx.used_size += nv_calc_kv_size(kv);
    
    log_i("[NV]", "Set key: %s, value_len: %d, timestamp: %lu", key, value_len, kv->timestamp);
    
    // 自动保存
    if (g_nv_ctx.config.auto_save) {
        uint32_t current_time = Time_GetCurrentMs();
        if (current_time - g_nv_ctx.last_save_time >= g_nv_ctx.config.auto_save_interval) {
            nv_save_to_flash();
        }
    }
    
    return NV_SUCCESS;
}

nv_result_t nv_get(const char* key, void* value, uint16_t* value_len) {
    if (!g_nv_ctx.initialized) {
        log_e("[NV]", "NV library not initialized");
        return NV_ERROR_NOT_INITIALIZED;
    }
    
    if (key == NULL || value == NULL || value_len == NULL) {
        log_e("[NV]", "Invalid parameters");
        return NV_ERROR_INVALID_PARAM;
    }
    
    int16_t index = nv_find_key_index(key);
    if (index < 0) {
        log_d("[NV]", "Key not found: %s", key);
        return NV_ERROR_KEY_NOT_FOUND;
    }
    
    nv_kv_pair_t* kv = &g_nv_ctx.kv_pairs[index];
    
    // 检查缓冲区大小
    if (*value_len < kv->value_len) {
        log_e("[NV]", "Buffer too small for key: %s, required: %d, provided: %d", 
              key, kv->value_len, *value_len);
        *value_len = kv->value_len;
        return NV_ERROR_VALUE_TOO_LONG;
    }
    
    // CRC校验
    uint16_t calc_crc = nv_crc16(kv->value, kv->value_len);
    if (calc_crc != kv->crc) {
        log_e("[NV]", "CRC check failed for key: %s", key);
        return NV_ERROR_CRC_CHECK_FAILED;
    }
    
    // 复制数据
    memcpy(value, kv->value, kv->value_len);
    *value_len = kv->value_len;
    
    log_d("[NV]", "Get key: %s, value_len: %d", key, kv->value_len);
    return NV_SUCCESS;
}

nv_result_t nv_delete(const char* key) {
    if (!g_nv_ctx.initialized) {
        log_e("[NV]", "NV library not initialized");
        return NV_ERROR_NOT_INITIALIZED;
    }
    
    if (key == NULL) {
        log_e("[NV]", "Invalid key parameter");
        return NV_ERROR_INVALID_PARAM;
    }
    
    int16_t index = nv_find_key_index(key);
    if (index < 0) {
        log_d("[NV]", "Key not found for deletion: %s", key);
        return NV_ERROR_KEY_NOT_FOUND;
    }
    
    nv_kv_pair_t* kv = &g_nv_ctx.kv_pairs[index];
    uint32_t kv_size = nv_calc_kv_size(kv);
    
    // 移动后续元素
    for (uint16_t i = index; i < g_nv_ctx.kv_count - 1; i++) {
        memcpy(&g_nv_ctx.kv_pairs[i], &g_nv_ctx.kv_pairs[i + 1], sizeof(nv_kv_pair_t));
    }
    
    g_nv_ctx.kv_count--;
    g_nv_ctx.used_size -= kv_size;
    
    log_i("[NV]", "Deleted key: %s", key);
    
    // 自动保存
    if (g_nv_ctx.config.auto_save) {
        nv_save_to_flash();
    }
    
    return NV_SUCCESS;
}

bool nv_exists(const char* key) {
    if (!g_nv_ctx.initialized || key == NULL) {
        return false;
    }
    
    return nv_find_key_index(key) >= 0;
}

nv_result_t nv_get_keys(char keys[][NV_MAX_KEY_LEN], uint16_t max_keys, uint16_t* key_count) {
    if (!g_nv_ctx.initialized) {
        return NV_ERROR_NOT_INITIALIZED;
    }
    
    if (keys == NULL || key_count == NULL) {
        return NV_ERROR_INVALID_PARAM;
    }
    
    uint16_t count = (g_nv_ctx.kv_count < max_keys) ? g_nv_ctx.kv_count : max_keys;
    
    for (uint16_t i = 0; i < count; i++) {
        strncpy(keys[i], g_nv_ctx.kv_pairs[i].key, NV_MAX_KEY_LEN - 1);
        keys[i][NV_MAX_KEY_LEN - 1] = '\0';
    }
    
    *key_count = count;
    return NV_SUCCESS;
}

nv_result_t nv_get_count(uint16_t* count) {
    if (!g_nv_ctx.initialized) {
        return NV_ERROR_NOT_INITIALIZED;
    }
    
    if (count == NULL) {
        return NV_ERROR_INVALID_PARAM;
    }
    
    *count = g_nv_ctx.kv_count;
    return NV_SUCCESS;
}

nv_result_t nv_clear_all(void) {
    if (!g_nv_ctx.initialized) {
        return NV_ERROR_NOT_INITIALIZED;
    }
    
    g_nv_ctx.kv_count = 0;
    g_nv_ctx.used_size = 0;
    
    log_i("[NV]", "Cleared all KV pairs");
    
    // 自动保存
    if (g_nv_ctx.config.auto_save) {
        nv_save_to_flash();
    }
    
    return NV_SUCCESS;
}

nv_result_t nv_save_to_flash(void) {
    if (!g_nv_ctx.initialized) {
        return NV_ERROR_NOT_INITIALIZED;
    }
    
    hi_u32 ret = hi_nv_write(0x01, &g_nv_ctx, sizeof(nv_context_t), 0);
    if (ret != HI_ERR_SUCCESS) {
        log_e("[NV]", "Failed to save to flash: 0x%X", ret);
        return NV_ERROR_WRITE_FAILED;
    }
    
    g_nv_ctx.last_save_time = Time_GetCurrentMs();
    log_i("[NV]", "Saved %d KV pairs to flash", g_nv_ctx.kv_count);
    
    return NV_SUCCESS;
}

nv_result_t nv_load_from_flash(void) {
    if (!g_nv_ctx.initialized) {
        return NV_ERROR_NOT_INITIALIZED;
    }
    
    nv_context_t temp_ctx;
    hi_u32 ret = hi_nv_read(0x01, &temp_ctx, sizeof(nv_context_t), 0);
    if (ret != HI_ERR_SUCCESS) {
        log_w("[NV]", "Failed to load from flash: 0x%X", ret);
        return NV_ERROR_READ_FAILED;
    }
    
    // 验证加载的数据
    if (temp_ctx.kv_count > NV_MAX_KV_PAIRS) {
        log_e("[NV]", "Invalid KV count in flash: %d", temp_ctx.kv_count);
        return NV_ERROR_READ_FAILED;
    }
    
    memcpy(&g_nv_ctx, &temp_ctx, sizeof(nv_context_t));
    log_i("[NV]", "Loaded %d KV pairs from flash", g_nv_ctx.kv_count);
    
    return NV_SUCCESS;
}

nv_result_t nv_get_status(uint32_t* used_size, uint32_t* free_size, uint32_t* total_size) {
    if (!g_nv_ctx.initialized) {
        return NV_ERROR_NOT_INITIALIZED;
    }
    
    if (used_size) *used_size = g_nv_ctx.used_size;
    if (free_size) *free_size = g_nv_ctx.config.total_size - g_nv_ctx.used_size;
    if (total_size) *total_size = g_nv_ctx.config.total_size;
    
    return NV_SUCCESS;
}

const char* nv_get_version(void) {
    return NV_LIB_VERSION;
}

void nv_set_auto_save(bool enable, uint32_t interval) {
    g_nv_ctx.config.auto_save = enable;
    g_nv_ctx.config.auto_save_interval = interval;
    
    log_i("[NV]", "Auto save %s, interval: %lu ms", enable ? "enabled" : "disabled", interval);
}