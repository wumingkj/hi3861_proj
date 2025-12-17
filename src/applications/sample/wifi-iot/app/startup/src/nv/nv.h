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

#ifndef NV_H
#define NV_H

#include <hi_types.h>
#include <stdint.h>
#include <stdbool.h>

#include "../debug.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

// NV库版本
#define NV_LIB_VERSION "1.0.0"

// KV存储的最大键长度
#define NV_MAX_KEY_LEN 32
// KV存储的最大值长度
#define NV_MAX_VALUE_LEN 128
// 最大KV对数量
#define NV_MAX_KV_PAIRS 50

// NV操作结果枚举
typedef enum {
    NV_SUCCESS = 0,                    // 成功
    NV_ERROR_INIT_FAILED,              // 初始化失败
    NV_ERROR_KEY_TOO_LONG,             // 键过长
    NV_ERROR_VALUE_TOO_LONG,           // 值过长
    NV_ERROR_STORAGE_FULL,             // 存储空间已满
    NV_ERROR_KEY_NOT_FOUND,            // 键不存在
    NV_ERROR_READ_FAILED,              // 读取失败
    NV_ERROR_WRITE_FAILED,             // 写入失败
    NV_ERROR_INVALID_PARAM,            // 参数无效
    NV_ERROR_NOT_INITIALIZED,          // 未初始化
    NV_ERROR_CRC_CHECK_FAILED,         // CRC校验失败
} nv_result_t;

// KV对结构体
typedef struct {
    char key[NV_MAX_KEY_LEN];          // 键
    uint8_t value[NV_MAX_VALUE_LEN];   // 值
    uint16_t value_len;                // 值长度
    uint32_t timestamp;                // 时间戳
    uint16_t crc;                      // CRC校验
} nv_kv_pair_t;

// NV配置结构体
typedef struct {
    uint32_t base_addr;                // NV存储基地址
    uint32_t total_size;               // 总大小
    uint32_t block_size;               // 块大小
    bool auto_save;                    // 自动保存
    uint32_t auto_save_interval;       // 自动保存间隔(ms)
} nv_config_t;

/**
 * @brief 初始化NV库
 * @param config NV配置参数
 * @return 初始化结果
 */
nv_result_t nv_init(const nv_config_t* config);

/**
 * @brief 反初始化NV库
 */
void nv_deinit(void);

/**
 * @brief 设置KV值
 * @param key 键
 * @param value 值
 * @param value_len 值长度
 * @return 操作结果
 */
nv_result_t nv_set(const char* key, const void* value, uint16_t value_len);

/**
 * @brief 获取KV值
 * @param key 键
 * @param value 值缓冲区
 * @param value_len 值长度指针（输入为缓冲区大小，输出为实际值长度）
 * @return 操作结果
 */
nv_result_t nv_get(const char* key, void* value, uint16_t* value_len);

/**
 * @brief 删除KV对
 * @param key 键
 * @return 操作结果
 */
nv_result_t nv_delete(const char* key);

/**
 * @brief 检查键是否存在
 * @param key 键
 * @return true-存在, false-不存在
 */
bool nv_exists(const char* key);

/**
 * @brief 获取所有键列表
 * @param keys 键列表缓冲区
 * @param max_keys 最大键数量
 * @param key_count 实际键数量
 * @return 操作结果
 */
nv_result_t nv_get_keys(char keys[][NV_MAX_KEY_LEN], uint16_t max_keys, uint16_t* key_count);

/**
 * @brief 获取KV对数量
 * @param count 数量指针
 * @return 操作结果
 */
nv_result_t nv_get_count(uint16_t* count);

/**
 * @brief 清空所有KV对
 * @return 操作结果
 */
nv_result_t nv_clear_all(void);

/**
 * @brief 手动保存到Flash
 * @return 操作结果
 */
nv_result_t nv_save_to_flash(void);

/**
 * @brief 从Flash加载数据
 * @return 操作结果
 */
nv_result_t nv_load_from_flash(void);

/**
 * @brief 获取NV库状态信息
 * @param used_size 已使用大小
 * @param free_size 剩余大小
 * @param total_size 总大小
 * @return 操作结果
 */
nv_result_t nv_get_status(uint32_t* used_size, uint32_t* free_size, uint32_t* total_size);

/**
 * @brief 获取NV库版本
 * @return 版本字符串
 */
const char* nv_get_version(void);

/**
 * @brief 设置自动保存
 * @param enable 是否启用
 * @param interval 保存间隔(ms)
 */
void nv_set_auto_save(bool enable, uint32_t interval);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* NV_H */