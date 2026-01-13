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

#include "kv.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>  // 添加stdlib.h以使用atof函数
#include <math.h>    // 用于isnan和isinf函数
#include <limits.h>  // 用于UINT32_MAX

// 模块状态
static bool g_kv_initialized = false;

// KV存储版本
static const char* g_kv_version = KV_LIB_VERSION;

kv_result_t kv_init(void) {
    if (g_kv_initialized) {
        return KV_SUCCESS;
    }
    // LiteOS KV存储系统不需要显式初始化
    // 直接使用UtilsSetValue/UtilsGetValue即可
    g_kv_initialized = true;
    return KV_SUCCESS;
}

void kv_deinit(void) {
    g_kv_initialized = false;
}

kv_result_t kv_set_string(const char* key, const char* value) {
    if (!g_kv_initialized) {
        return KV_ERROR_NOT_INITIALIZED;
    }
    
    if (key == NULL || value == NULL) {
        return KV_ERROR_INVALID_PARAM;
    }
    
    // 检查键长度
    size_t key_len = strlen(key);
    if (key_len == 0 || key_len > 31) {  // LiteOS KV键最大32字节（包括结束符）
        return KV_ERROR_KEY_TOO_LONG;
    }
    
    // 检查值长度
    size_t value_len = strlen(value);
    if (value_len > 127) {  // LiteOS KV值最大128字节（包括结束符）
        return KV_ERROR_VALUE_TOO_LONG;
    }
    
    // 使用LiteOS KV存储系统
    int ret = UtilsSetValue(key, value);
    if (ret == 0) {
        return KV_SUCCESS;
    } else if (ret == -9) {
        return KV_ERROR_INVALID_PARAM;
    } else {
        return KV_ERROR_WRITE_FAILED;
    }
}

kv_result_t kv_get_string(const char* key, char* value, uint16_t max_len) {
    if (!g_kv_initialized) {
        return KV_ERROR_NOT_INITIALIZED;
    }
    
    if (key == NULL || value == NULL || max_len == 0) {
        return KV_ERROR_INVALID_PARAM;
    }
    
    // 检查键长度
    size_t key_len = strlen(key);
    if (key_len == 0 || key_len > 31) {
        return KV_ERROR_KEY_TOO_LONG;
    }
    
    // 使用LiteOS KV存储系统
    int ret = UtilsGetValue(key, value, max_len);
    if (ret > 0) {
        // 确保字符串以null结尾
        if (ret < max_len) {
            value[ret] = '\0';
        } else {
            value[max_len - 1] = '\0';
        }
        return KV_SUCCESS;
    } else if (ret == -9) {
        return KV_ERROR_INVALID_PARAM;
    } else if (ret == -1) {
        return KV_ERROR_KEY_NOT_FOUND;
    } else {
        return KV_ERROR_READ_FAILED;
    }
}

kv_result_t kv_set_uint32(const char* key, uint32_t value) {
    if (!g_kv_initialized) {
        return KV_ERROR_NOT_INITIALIZED;
    }
    
    if (key == NULL) {
        return KV_ERROR_INVALID_PARAM;
    }
    
    // 将uint32转换为字符串存储 - 使用snprintf确保安全
    char value_str[16];
    int len = snprintf(value_str, sizeof(value_str), "%u", value);
    
    // 检查转换是否成功
    if (len < 0) {
        return KV_ERROR_WRITE_FAILED;
    }
    
    // 检查缓冲区是否足够
    if (len >= (int)sizeof(value_str)) {
        return KV_ERROR_VALUE_TOO_LONG;
    }
    
    return kv_set_string(key, value_str);
}
kv_result_t kv_get_uint32(const char* key, uint32_t* value) {
    if (!g_kv_initialized) {
        return KV_ERROR_NOT_INITIALIZED;
    }
    
    if (key == NULL || value == NULL) {
        return KV_ERROR_INVALID_PARAM;
    }
    
    // 读取字符串值
    char value_str[16];
    kv_result_t ret = kv_get_string(key, value_str, sizeof(value_str));
    if (ret != KV_SUCCESS) {
        return ret;
    }
    
    // 检查字符串是否为空
    if (value_str[0] == '\0') {
        return KV_ERROR_READ_FAILED;
    }
    
    // 转换为uint32 - 使用strtoul
    char* endptr;
    unsigned long tmp = strtoul(value_str, &endptr, 10);
    
    // 检查转换是否成功
    if (endptr == value_str || *endptr != '\0') {
        return KV_ERROR_READ_FAILED;
    }
    
    // 检查是否溢出
    if (tmp > UINT32_MAX) {
        return KV_ERROR_READ_FAILED;
    }
    
    // 检查转换后的值是否合理（不能为0，除非原始字符串就是"0"）
    if (tmp == 0 && strcmp(value_str, "0") != 0) {
        return KV_ERROR_READ_FAILED;
    }
    
    *value = (uint32_t)tmp;
    return KV_SUCCESS;
}

kv_result_t kv_set_float(const char* key, float value) {
    if (!g_kv_initialized) {
        return KV_ERROR_NOT_INITIALIZED;
    }
    
    if (key == NULL) {
        return KV_ERROR_INVALID_PARAM;
    }
    
    // 检查浮点数是否有效
    if (isnan(value) || isinf(value)) {
        return KV_ERROR_INVALID_PARAM;
    }
    
    // 将float转换为字符串存储 - 使用snprintf确保安全
    char value_str[32];
    int len = snprintf(value_str, sizeof(value_str), "%.2f", value);
    
    // 检查转换是否成功
    if (len < 0) {
        return KV_ERROR_WRITE_FAILED;
    }
    
    // 检查缓冲区是否足够
    if (len >= (int)sizeof(value_str)) {
        return KV_ERROR_VALUE_TOO_LONG;
    }
    
    return kv_set_string(key, value_str);
}
kv_result_t kv_get_float(const char* key, float* value) {
    if (!g_kv_initialized) {
        return KV_ERROR_NOT_INITIALIZED;
    }
    
    if (key == NULL || value == NULL) {
        return KV_ERROR_INVALID_PARAM;
    }
    
    // 读取字符串值
    char value_str[32];
    kv_result_t ret = kv_get_string(key, value_str, sizeof(value_str));
    if (ret != KV_SUCCESS) {
        return ret;
    }
    
    // 检查字符串是否为空
    if (value_str[0] == '\0') {
        return KV_ERROR_READ_FAILED;
    }
    
    // 转换为float - 使用strtof代替atof，提供错误检查
    char* endptr;
    float tmp = strtof(value_str, &endptr);
    
    // 检查转换是否成功
    if (endptr == value_str || *endptr != '\0') {
        return KV_ERROR_READ_FAILED;
    }
    
    // 检查是否为NaN或无穷大
    if (isnan(tmp) || isinf(tmp)) {
        return KV_ERROR_READ_FAILED;
    }
    
    // 检查转换后的值是否合理（不能为0，除非原始字符串就是"0"或"0.0"等）
    if (tmp == 0.0f) {
        // 检查原始字符串是否为有效的0表示
        int is_valid_zero = 0;
        if (strcmp(value_str, "0") == 0 || 
            strcmp(value_str, "0.0") == 0 || 
            strcmp(value_str, "0.00") == 0 ||
            strcmp(value_str, "0.000") == 0) {
            is_valid_zero = 1;
        }
        
        // 如果不是有效的0表示，则认为是转换错误
        if (!is_valid_zero) {
            return KV_ERROR_READ_FAILED;
        }
    }
    
    *value = tmp;
    return KV_SUCCESS;
}

kv_result_t kv_delete(const char* key) {
    if (!g_kv_initialized) {
        return KV_ERROR_NOT_INITIALIZED;
    }
    
    if (key == NULL) {
        return KV_ERROR_INVALID_PARAM;
    }
    
    // 检查键长度
    size_t key_len = strlen(key);
    if (key_len == 0 || key_len > 31) {
        return KV_ERROR_KEY_TOO_LONG;
    }
    
    // 使用LiteOS KV存储系统
    int ret = UtilsDeleteValue(key);
    if (ret == 0) {
        return KV_SUCCESS;
    } else if (ret == -9) {
        return KV_ERROR_INVALID_PARAM;
    } else {
        return KV_ERROR_WRITE_FAILED;
    }
}

bool kv_exists(const char* key) {
    if (!g_kv_initialized || key == NULL) {
        return false;
    }
    
    // 检查键长度
    size_t key_len = strlen(key);
    if (key_len == 0 || key_len > 31) {
        return false;
    }
    
    // 尝试读取值来判断键是否存在
    char dummy[1];
    int ret = UtilsGetValue(key, dummy, 1);
    return (ret > 0);
}

kv_result_t kv_clear_all(void) {
    if (!g_kv_initialized) {
        return KV_ERROR_NOT_INITIALIZED;
    }
    // LiteOS KV存储系统没有提供清空所有键值对的API
    // 这里需要手动删除所有已知的键
    // 在实际应用中，应该维护一个键列表
    return KV_SUCCESS;
}

const char* kv_get_version(void) {
    return g_kv_version;
}