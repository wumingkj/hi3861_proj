/**
 ****************************************************************************************************
 * @file        template.c
 * @author      普中科技
 * @version     V1.1
 * @date        2024-06-05
 * @brief       NFC初始化与检测程序
 * @license     Copyright (c) 2024-2034, 深圳市普中科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:普中-Hi3861
 * 在线视频:https://space.bilibili.com/2146492485
 * 公司网址:www.prechin.cn
 * 购买地址:
 *
 ****************************************************************************************************
 * 实验现象：读取NFC标签中的WIFI配置并解析SSID和密码
 *
 ****************************************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "ohos_init.h"
#include "cmsis_os2.h"

#include "nfc.h"
// 清理NFC状态
void nfc_cleanup(void)
{
    printf("\n[初始化] 开始清理NFC状态...\n");
    
    // 1. 初始化NFC
    printf("[NFC] 初始化NFC模块...\n");
    nfc_init();
    
    // 2. 延时确保初始化完成
    usleep(1000 * 1000);
    
    printf("[初始化] NFC状态清理完成\n\n");
}

// 解析WIFI URI格式: WIFI:S:<SSID>;T:<TYPE>;P:<PASSWORD>;;
void parse_wifi_config(const char* wifi_uri)
{
    printf("\n[WIFI配置解析]:\n");
    
    char ssid[64] = {0};
    char password[64] = {0};
    char auth_type[32] = {0};
    
    // 跳过"WIFI:"前缀
    const char* ptr = wifi_uri + 5;
    
    while (*ptr != '\0') {
        // 查找分隔符
        if (*ptr == ';') {
            ptr++;
            continue;
        }
        
        // 解析字段
        if (*ptr == 'S' && *(ptr + 1) == ':') {
            // 解析SSID
            const char* start = ptr + 2;
            int i = 0;
            while (*start != ';' && *start != '\0') {
                ssid[i++] = *start++;
            }
            ssid[i] = '\0';
            printf("   SSID: %s\n", ssid);
            ptr = start;
        }
        else if (*ptr == 'T' && *(ptr + 1) == ':') {
            // 解析认证类型
            const char* start = ptr + 2;
            int i = 0;
            while (*start != ';' && *start != '\0') {
                auth_type[i++] = *start++;
            }
            auth_type[i] = '\0';
            printf("   认证类型: %s\n", auth_type);
            ptr = start;
        }
        else if (*ptr == 'P' && *(ptr + 1) == ':') {
            // 解析密码
            const char* start = ptr + 2;
            int i = 0;
            while (*start != ';' && *start != '\0') {
                password[i++] = *start++;
            }
            password[i] = '\0';
            
            // 显示密码（可以用*号隐藏）
            printf("   密码: ");
            for (int j = 0; j < i; j++) {
                printf("*");
            }
            printf(" (长度: %d)\n", i);
            
            // 如果调试需要显示明文密码，可以取消下面注释
            // printf("   明文密码: %s\n", password);
            
            ptr = start;
        }
        else {
            ptr++;
        }
    }
    
    printf("\n[连接信息]:\n");
    printf("   WIFI名称: %s\n", ssid);
    printf("   加密方式: %s\n", auth_type);
    if (strlen(password) > 0) {
        printf("   密码长度: %d 字符\n", strlen(password));
        printf("   ✅ 需要密码连接\n");
    } else {
        printf("   ⚠ 开放网络，无需密码\n");
    }
    
    // 如果是开放网络，检查是否有问题
    if (strcmp(auth_type, "NONE") == 0 && strlen(password) > 0) {
        printf("   ⚠ 警告: 认证类型为NONE但检测到密码\n");
    }
}

// 检查NFC标签并读取WIFI配置
void read_nfc_wifi_config(void)
{
    printf("[检测] 开始检测NFC标签...\n");
    
    int max_attempts = 3;
    for (int attempt = 1; attempt <= max_attempts; attempt++) {
        printf("[检测] 尝试 %d/%d，请将NFC标签靠近开发板...\n", attempt, max_attempts);
        
        uint8_t ndefLen = 0;
        uint8_t ndef_Header = 0;
        uint32_t result_code = 0;
        
        // 尝试读取NDEF头信息
        result_code = NT3HReadHeaderNfc(&ndefLen, &ndef_Header);
        
        if (result_code == true) {
            printf("\n✅ NFC标签检测成功！\n");
            printf("   NDEF数据长度: %d 字节\n", ndefLen);
            
            // 尝试读取NDEF数据
            if (ndefLen > 0) {
                ndefLen += NDEF_HEADER_SIZE;  // 加上头大小
                printf("\n[读取NDEF数据] 数据长度: %d 字节\n", ndefLen);
                
                uint8_t *ndefBuff = (uint8_t *)malloc(ndefLen + 1);
                if (ndefBuff != NULL) {
                    result_code = get_NDEFDataPackage(ndefBuff, ndefLen);
                    if (result_code == HI_ERR_SUCCESS) {
                        printf("   ✅ NDEF数据读取成功\n");
                        
                        // 查找WIFI:前缀
                        int wifi_found = 0;
                        for (uint32_t i = 0; i < ndefLen - 5 && !wifi_found; i++) {
                            if (memcmp(&ndefBuff[i], "WIFI:", 5) == 0) {
                                wifi_found = 1;
                                
                                // 提取整个WIFI配置字符串
                                char wifi_config[256] = {0};
                                int j = 0;
                                
                                // 从WIFI:开始复制
                                for (uint32_t k = i; k < ndefLen && j < 255; k++) {
                                    wifi_config[j++] = ndefBuff[k];
                                    
                                    // 检查是否到达配置结束（两个分号）
                                    if (k > i + 10 && ndefBuff[k] == ';' && k + 1 < ndefLen && ndefBuff[k + 1] == ';') {
                                        wifi_config[j++] = ';';  // 添加第二个分号
                                        wifi_config[j] = '\0';   // 结束字符串
                                        break;
                                    }
                                }
                                
                                if (j > 0) {
                                    wifi_config[j] = '\0';
                                    
                                    printf("\n🎉 检测到WIFI配置！\n");
                                    printf("   完整配置: %s\n", wifi_config);
                                    
                                    // 解析并打印SSID和密码
                                    parse_wifi_config(wifi_config);
                                } else {
                                    printf("   ❌ 无法提取WIFI配置字符串\n");
                                }
                                
                                break;
                            }
                        }
                        
                        if (!wifi_found) {
                            printf("\n⚠ 未检测到WIFI配置\n");
                            
                            // 显示读取到的内容用于调试
                            printf("\n[读取到的原始数据预览]:\n");
                            printf("   ");
                            for (int i = 0; i < ndefLen && i < 50; i++) {
                                if (ndefBuff[i] >= 32 && ndefBuff[i] <= 126) {
                                    printf("%c", ndefBuff[i]);
                                } else {
                                    printf(".");
                                }
                            }
                            if (ndefLen > 50) printf("...");
                            printf("\n");
                        }
                    } else {
                        printf("   ❌ NDEF数据读取失败，错误码: %d\n", result_code);
                    }
                    
                    free(ndefBuff);
                } else {
                    printf("   ❌ 内存分配失败\n");
                }
            } else {
                printf("\n⚠ 标签中没有NDEF数据\n");
            }
            
            printf("\n[操作完成] 读取成功，等待下一个标签...\n");
            printf("========================================\n");
            
            return;  // 检测成功，退出
        } else {
            printf("   ❌ 第 %d 次尝试失败\n", attempt);
            
            // 等待后重试
            if (attempt < max_attempts) {
                printf("   等待2秒后重试...\n\n");
                usleep(2000 * 1000);
            }
        }
    }
    
    printf("\n❌ NFC标签检测失败\n");
    printf("   可能原因:\n");
    printf("   1. 没有放置NFC标签\n");
    printf("   2. 标签距离过远\n");
    printf("   3. 标签是空的\n");
}

// 主任务：持续检测NFC标签并读取WIFI配置
void NFC_Detect_Task(void)
{
    printf("\n[任务] NFC WIFI配置读取任务启动\n");
    printf("========================================\n");
    
    while (1) {
        read_nfc_wifi_config();
        
        printf("\n[系统] 等待5秒后重新检测...\n");
        printf("[倒计时] 5 秒...\n");
        usleep(1000 * 1000);
        printf("[倒计时] 4 秒...\n");
        usleep(1000 * 1000);
        printf("[倒计时] 3 秒...\n");
        usleep(1000 * 1000);
        printf("[倒计时] 2 秒...\n");
        usleep(1000 * 1000);
        printf("[倒计时] 1 秒...\n");
        usleep(1000 * 1000);
        
        printf("\n========================================\n");
        printf("准备读取下一个NFC标签...\n");
        printf("========================================\n\n");
    }
}

// NFC检测任务创建
void nfc_detect_task_create(void)
{
    osThreadAttr_t taskOptions;
    taskOptions.name = "NFCDetect";
    taskOptions.attr_bits = 0;
    taskOptions.cb_mem = NULL;
    taskOptions.cb_size = 0;
    taskOptions.stack_mem = NULL;
    taskOptions.stack_size = 1024 * 4;
    taskOptions.priority = osPriorityNormal;

    osThreadId_t nfcTaskID = osThreadNew((osThreadFunc_t)NFC_Detect_Task, NULL, &taskOptions);
    if (nfcTaskID != NULL) {
        printf("[系统] NFC检测任务创建成功，ID = %d\n", nfcTaskID);
    }
}

// 初始化入口
static void nfc_init_demo(void)
{
    printf("\n");
    printf("****************************************************\n");
    printf("*    NFC WIFI配置读取程序 V1.2                    *\n");
    printf("*    读取并解析NFC标签中的WIFI配置                *\n");
    printf("****************************************************\n");
    printf("*    格式: WIFI:S:SSID;T:TYPE;P:PASSWORD;;        *\n");
    printf("****************************************************\n");
    printf("\n");
    // 2. 清理NFC状态
    nfc_cleanup();
    
    // 3. 创建NFC检测任务
    nfc_detect_task_create();
    
    printf("\n[系统] 所有任务已启动，系统运行中...\n");
    printf("请将包含WIFI配置的NFC标签靠近开发板\n");
    printf("========================================\n");
}

// 注册系统启动任务
SYS_RUN(nfc_init_demo);