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

#ifndef KV_H
#define KV_H

#include <stdint.h>
#include <stdbool.h>
#include "kv_store.h"  // LiteOS KV存储系统

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

// KV存储版本
#define KV_LIB_VERSION "1.0.0"

// KV操作结果枚举
typedef enum {
    KV_SUCCESS = 0,                    // 成功
    KV_ERROR_INIT_FAILED,              // 初始化失败
    KV_ERROR_KEY_TOO_LONG,             // 键过长
    KV_ERROR_VALUE_TOO_LONG,           // 值过长
    KV_ERROR_STORAGE_FULL,             // 存储空间已满
    KV_ERROR_KEY_NOT_FOUND,            // 键不存在
    KV_ERROR_READ_FAILED,              // 读取失败
    KV_ERROR_WRITE_FAILED,             // 写入失败
    KV_ERROR_INVALID_PARAM,            // 参数无效
    KV_ERROR_NOT_INITIALIZED,          // 未初始化
} kv_result_t;

/**
 * @brief 初始化KV存储模块
 * @return 初始化结果
 */
kv_result_t kv_init(void);

/**
 * @brief 反初始化KV存储模块
 */
void kv_deinit(void);

/**
 * @brief 设置字符串类型的值
 * @param key 键
 * @param value 字符串值
 * @return 操作结果
 */
kv_result_t kv_set_string(const char* key, const char* value);

/**
 * @brief 获取字符串类型的值
 * @param key 键
 * @param value 字符串缓冲区
 * @param max_len 最大长度
 * @return 操作结果
 */
kv_result_t kv_get_string(const char* key, char* value, uint16_t max_len);

/**
 * @brief 设置uint32类型的值
 * @param key 键
 * @param value 值
 * @return 操作结果
 */
kv_result_t kv_set_uint32(const char* key, uint32_t value);

/**
 * @brief 获取uint32类型的值
 * @param key 键
 * @param value 值指针
 * @return 操作结果
 */
kv_result_t kv_get_uint32(const char* key, uint32_t* value);

/**
 * @brief 删除键值对
 * @param key 键
 * @return 操作结果
 */
kv_result_t kv_delete(const char* key);

/**
 * @brief 检查键是否存在
 * @param key 键
 * @return true-存在, false-不存在
 */
bool kv_exists(const char* key);

/**
 * @brief 清空所有键值对
 * @return 操作结果
 */
kv_result_t kv_clear_all(void);

/**
 * @brief 获取KV存储模块版本
 * @return 版本字符串
 */
const char* kv_get_version(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif // KV_H