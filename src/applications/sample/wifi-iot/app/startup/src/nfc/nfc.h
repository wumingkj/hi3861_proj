#ifndef NFC_H
#define NFC_H

#include "NT3H.h"
#include <stdint.h>
#include <stdbool.h>

// NFC状态枚举
typedef enum {
    NFC_STATE_DISCONNECTED = 0,  // 未连接
    NFC_STATE_CONNECTED,         // 已连接
    NFC_STATE_READING,           // 正在读取
    NFC_STATE_WRITING,           // 正在写入
    NFC_STATE_ERROR              // 错误状态
} nfc_state_t;

// NFC信息结构体
typedef struct {
    uint8_t tag_type;           // 标签类型
    uint8_t memory_size;        // 内存大小(KB)
    uint8_t serial_number[7];   // 序列号
    uint8_t manufacturer_data[8]; // 制造商数据
    uint16_t user_memory_size;  // 用户内存大小(字节)
} nfc_info_t;

// KV交互回调函数类型
typedef void (*nfc_kv_callback_t)(const char* key, const char* value);

/*
 * The function write in the NT3H a new URI Rtd on the required position
 */
bool storeUrihttp(RecordPosEnu position, uint8_t *http);

/*
 * The function write in the NT3H a new Text Rtd on the required position
 */
bool storeText(RecordPosEnu position, uint8_t *text);

// ========== 新增功能接口 ==========

/**
 * @brief 获取NFC当前状态
 * @return nfc_state_t NFC状态
 */
nfc_state_t nfc_get_state(void);

/**
 * @brief 获取NFC标签信息
 * @param info 存储NFC信息的结构体指针
 * @return bool 成功返回true，失败返回false
 */
bool nfc_get_info(nfc_info_t *info);

/**
 * @brief 读取NFC标签中的所有文本记录
 * @param buffer 存储文本的缓冲区
 * @param buffer_size 缓冲区大小
 * @return int 实际读取的字节数，失败返回-1
 */
int nfc_read_all_text(char *buffer, uint16_t buffer_size);

/**
 * @brief 读取NFC标签中的所有URI记录
 * @param buffer 存储URI的缓冲区
 * @param buffer_size 缓冲区大小
 * @return int 实际读取的字节数，失败返回-1
 */
int nfc_read_all_uri(char *buffer, uint16_t buffer_size);

/**
 * @brief 检测NFC标签是否就绪
 * @return bool 就绪返回true，否则返回false
 */
bool nfc_is_ready(void);

/**
 * @brief 获取最后一次错误代码
 * @return uint32_t 错误代码
 */
uint32_t nfc_get_last_error(void);

/**
 * @brief 获取错误信息描述
 * @param error_code 错误代码
 * @return const char* 错误描述字符串
 */
const char* nfc_get_error_string(uint32_t error_code);

// ========== KV交互抽象接口 ==========

/**
 * @brief 初始化KV交互模块
 * @param callback KV数据回调函数
 * @return bool 初始化成功返回true
 */
bool nfc_kv_init(nfc_kv_callback_t callback);

/**
 * @brief 从NFC标签读取KV数据并同步到系统KV存储
 * @return bool 同步成功返回true
 */
bool nfc_kv_sync_from_tag(void);

/**
 * @brief 将系统KV数据写入NFC标签
 * @return bool 写入成功返回true
 */
bool nfc_kv_sync_to_tag(void);

/**
 * @brief 读取NFC标签中的特定KV键值对
 * @param key 键名
 * @param value 值缓冲区
 * @param value_size 值缓冲区大小
 * @return bool 读取成功返回true
 */
bool nfc_kv_read(const char* key, char* value, uint16_t value_size);

/**
 * @brief 写入KV键值对到NFC标签
 * @param key 键名
 * @param value 值
 * @return bool 写入成功返回true
 */
bool nfc_kv_write(const char* key, const char* value);

/**
 * @brief 删除NFC标签中的KV键值对
 * @param key 键名
 * @return bool 删除成功返回true
 */
bool nfc_kv_delete(const char* key);

/**
 * @brief 获取NFC标签中KV键值对的数量
 * @return int 键值对数量，失败返回-1
 */
int nfc_kv_get_count(void);

// ========== 正确的NFC数据读取并保存到KV的接口 ==========

/**
 * @brief 处理NFC特定命令并读取数据保存到KV
 * @param command NFC命令数据
 * @param command_len 命令长度
 * @return bool 处理成功返回true，失败返回false
 */
bool nfc_process_command_and_save_kv(const uint8_t* command, uint16_t command_len);

/**
 * @brief 从NFC标签读取配置数据并保存到KV
 * @return bool 读取并保存成功返回true
 */
bool nfc_read_config_and_save_kv(void);

/**
 * @brief 从NFC标签读取用户数据并保存到KV
 * @return bool 读取并保存成功返回true
 */
bool nfc_read_user_data_and_save_kv(void);

/**
 * @brief 初始化NFC模块
 * @return bool 初始化成功返回true，失败返回false
 */
bool nfc_init(void);

#endif /* NFC_H_ */