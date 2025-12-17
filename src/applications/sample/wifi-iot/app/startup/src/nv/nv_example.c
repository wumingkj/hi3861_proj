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
#include "ohos_init.h"
#include "cmsis_os2.h"

#include "nv.h"
#include "../debug.h"
#include "time.h"

// NV配置
static nv_config_t g_nv_config = {
    .base_addr = 0xA000,           // 非工厂NV分区地址
    .total_size = 0x2000,          // 8KB总大小
    .block_size = 0x1000,          // 4KB块大小
    .auto_save = true,             // 启用自动保存
    .auto_save_interval = 5000,    // 5秒自动保存间隔
};

/**
 * @brief NV库使用示例
 */
void nv_library_example(void) {
    log_i("[NV_EXAMPLE]", "Starting NV library example");
    
    // 初始化NV库
    nv_result_t ret = nv_init(&g_nv_config);
    if (ret != NV_SUCCESS) {
        log_e("[NV_EXAMPLE]", "Failed to initialize NV library: %d", ret);
        return;
    }
    
    // 示例1: 设置各种类型的KV对
    log_i("[NV_EXAMPLE]", "=== Example 1: Setting KV pairs ===");
    
    // 设置整数
    int32_t temperature = 25;
    ret = nv_set("temperature", &temperature, sizeof(temperature));
    if (ret == NV_SUCCESS) {
        log_i("[NV_EXAMPLE]", "Set temperature: %d", temperature);
    }
    
    // 设置浮点数
    float humidity = 65.5f;
    ret = nv_set("humidity", &humidity, sizeof(humidity));
    if (ret == NV_SUCCESS) {
        log_i("[NV_EXAMPLE]", "Set humidity: %.1f", humidity);
    }
    
    // 设置字符串
    const char* device_name = "Hi3861_Device";
    ret = nv_set("device_name", device_name, strlen(device_name) + 1);
    if (ret == NV_SUCCESS) {
        log_i("[NV_EXAMPLE]", "Set device_name: %s", device_name);
    }
    
    // 设置布尔值
    bool wifi_enabled = true;
    ret = nv_set("wifi_enabled", &wifi_enabled, sizeof(wifi_enabled));
    if (ret == NV_SUCCESS) {
        log_i("[NV_EXAMPLE]", "Set wifi_enabled: %s", wifi_enabled ? "true" : "false");
    }
    
    // 设置配置结构体
    typedef struct {
        uint8_t version_major;
        uint8_t version_minor;
        uint16_t build_number;
    } device_config_t;
    
    device_config_t config = {1, 0, 1001};
    ret = nv_set("device_config", &config, sizeof(config));
    if (ret == NV_SUCCESS) {
        log_i("[NV_EXAMPLE]", "Set device_config: v%d.%d build %d", 
              config.version_major, config.version_minor, config.build_number);
    }
    
    // 示例2: 获取KV对
    log_i("[NV_EXAMPLE]", "=== Example 2: Getting KV pairs ===");
    
    int32_t read_temp;
    uint16_t temp_len = sizeof(read_temp);
    ret = nv_get("temperature", &read_temp, &temp_len);
    if (ret == NV_SUCCESS) {
        log_i("[NV_EXAMPLE]", "Get temperature: %d", read_temp);
    }
    
    float read_humidity;
    uint16_t humidity_len = sizeof(read_humidity);
    ret = nv_get("humidity", &read_humidity, &humidity_len);
    if (ret == NV_SUCCESS) {
        log_i("[NV_EXAMPLE]", "Get humidity: %.1f", read_humidity);
    }
    
    char read_device_name[50];
    uint16_t name_len = sizeof(read_device_name);
    ret = nv_get("device_name", read_device_name, &name_len);
    if (ret == NV_SUCCESS) {
        log_i("[NV_EXAMPLE]", "Get device_name: %s", read_device_name);
    }
    
    // 示例3: 检查键是否存在和删除
    log_i("[NV_EXAMPLE]", "=== Example 3: Key operations ===");
    
    if (nv_exists("wifi_enabled")) {
        log_i("[NV_EXAMPLE]", "Key 'wifi_enabled' exists");
        
        // 删除键
        ret = nv_delete("wifi_enabled");
        if (ret == NV_SUCCESS) {
            log_i("[NV_EXAMPLE]", "Deleted key 'wifi_enabled'");
        }
    }
    
    // 检查删除后的状态
    if (!nv_exists("wifi_enabled")) {
        log_i("[NV_EXAMPLE]", "Key 'wifi_enabled' successfully deleted");
    }
    
    // 示例4: 获取所有键列表
    log_i("[NV_EXAMPLE]", "=== Example 4: Key list ===");
    
    char key_list[10][NV_MAX_KEY_LEN];
    uint16_t key_count;
    ret = nv_get_keys(key_list, 10, &key_count);
    if (ret == NV_SUCCESS) {
        log_i("[NV_EXAMPLE]", "Total keys: %d", key_count);
        for (uint16_t i = 0; i < key_count; i++) {
            log_i("[NV_EXAMPLE]", "Key %d: %s", i + 1, key_list[i]);
        }
    }
    
    // 示例5: 获取库状态
    log_i("[NV_EXAMPLE]", "=== Example 5: Library status ===");
    
    uint32_t used_size, free_size, total_size;
    ret = nv_get_status(&used_size, &free_size, &total_size);
    if (ret == NV_SUCCESS) {
        log_i("[NV_EXAMPLE]", "Storage status: used=%d bytes, free=%d bytes, total=%d bytes", 
              used_size, free_size, total_size);
        log_i("[NV_EXAMPLE]", "Usage: %.1f%%", (float)used_size / total_size * 100);
    }
    
    // 示例6: 手动保存
    log_i("[NV_EXAMPLE]", "=== Example 6: Manual save ===");
    
    ret = nv_save_to_flash();
    if (ret == NV_SUCCESS) {
        log_i("[NV_EXAMPLE]", "Manual save completed");
    }
    
    log_i("[NV_EXAMPLE]", "NV library example completed");
    log_i("[NV_EXAMPLE]", "NV library version: %s", nv_get_version());
}

/**
 * @brief NV库测试任务
 */
static void nv_test_task(void* arg) {
    (void)arg;
    
    log_i("[NV_TEST]", "NV test task started");
    
    // 等待系统稳定
    Time_DelayMs(2000);
    
    // 运行示例
    nv_library_example();
    
    log_i("[NV_TEST]", "NV test task completed");
}

/**
 * @brief 创建NV测试任务
 */
void create_nv_test_task(void) {
    osThreadAttr_t task_attr = {
        .name = "NVTestTask",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U,
        .stack_mem = NULL,
        .stack_size = 0x1000,
        .priority = osPriorityNormal
    };
    
    if (osThreadNew((osThreadFunc_t)nv_test_task, NULL, &task_attr) == NULL) {
        log_e("[NV_TEST]", "Failed to create NV test task");
    } else {
        log_i("[NV_TEST]", "NV test task created successfully");
    }
}