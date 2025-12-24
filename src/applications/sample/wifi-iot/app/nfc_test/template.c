/**
 ****************************************************************************************************
 * @file        template.c
 * @author      普中科技
 * @version     V1.0
 * @date        2024-06-05
 * @brief       NFC实验
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
 * 实验现象：通过手机NFC APP向 NFC 标签中写入数据，并且读取 NFC 标签中的数据，通过串口助手输出。
 *
 ****************************************************************************************************
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "ohos_init.h"
#include "cmsis_os2.h"

#include "bsp_led.h"
#include "bsp_nfc.h"

// 添加NFC库头文件包含
#include "NT3H.h"
#include "nfcForum.h"

// 定义NDEF相关常量
#define NDEF_HEADER_SIZE 2
#define NDEF_START_BYTE 0x03

//LED任务
osThreadId_t LED_Task_ID; //led任务ID

void LED_Task(void)
{
    led_init();//LED初始化

    while (1) 
    {
        LED(1); 
        usleep(200*1000); //200ms
        LED(0);
        usleep(200*1000); //200ms
    }
}
//LED任务创建
void led_task_create(void)
{
    osThreadAttr_t taskOptions;
    taskOptions.name = "LEDTask";            // 任务的名字
    taskOptions.attr_bits = 0;               // 属性位
    taskOptions.cb_mem = NULL;               // 堆空间地址
    taskOptions.cb_size = 0;                 // 堆空间大小
    taskOptions.stack_mem = NULL;            // 栈空间地址
    taskOptions.stack_size = 1024;           // 栈空间大小 单位:字节
    taskOptions.priority = osPriorityNormal; // 任务的优先级

    LED_Task_ID = osThreadNew((osThreadFunc_t)LED_Task, NULL, &taskOptions); // 创建任务1
    if (LED_Task_ID != NULL)
    {
        printf("ID = %d, Create LED_Task_ID is OK!\n", LED_Task_ID);
    }
}

//NFC读取任务
osThreadId_t NFC_Read_Task_ID; //读取任务ID

void NFC_Read_Task(void) {
    uint8_t ndefLen = 0;      // ndef包的长度
    uint8_t ndef_Header = 0;  // ndef消息开始标志位-用不到
    uint32_t result_code = 0; // 函数的返回值
    uint8_t i=0;
    
    nfc_init();
    printf("NFC读取任务启动...\r\n");

    while (1) 
    {
        // 读整个数据的包头部分，读出整个数据的长度
        if ((result_code = NT3HReadHeaderNfc(&ndefLen, &ndef_Header)) != true) 
        {
            printf("NT3HReadHeaderNfc失败. result_code = %d\r\n", result_code);
            usleep(2000*1000); //2秒后重试
            continue;
        }

        ndefLen += NDEF_HEADER_SIZE; // 加上头部字节
        if (ndefLen <= NDEF_HEADER_SIZE) 
        {
            printf("ndefLen <= 2\r\n");
            usleep(2000*1000); //2秒后重试
            continue;
        }
        
        uint8_t *ndefBuff = (uint8_t *)malloc(ndefLen + 1);
        if (ndefBuff == NULL) 
        {
            printf("ndefBuff内存分配失败!\r\n");
            usleep(2000*1000); //2秒后重试
            continue;
        }

        if ((result_code = get_NDEFDataPackage(ndefBuff, ndefLen)) != HI_ERR_SUCCESS) 
        {
            printf("get_NDEFDataPackage失败. result_code = %d\r\n", result_code);
            free(ndefBuff);
            usleep(2000*1000); //2秒后重试
            continue;
        }

        printf("开始打印NDEF数据:\r\n");
        for (i = 0; i < ndefLen; i++) 
        {
            printf("0x%02x ", ndefBuff[i]);
            if ((i+1) % 16 == 0) printf("\r\n");
        }
        printf("\r\n");
        
        // 尝试解析文本记录
        if (ndefLen > 6) {
            // 检查是否为文本记录 (TNF=0x01)
            if (ndefBuff[0] == 0xD1) {
                uint8_t payloadLength = ndefBuff[1];
                uint8_t typeLength = ndefBuff[2];
                
                if (typeLength > 0 && payloadLength > 0) {
                    printf("文本记录类型: ");
                    for (i = 3; i < 3 + typeLength; i++) {
                        printf("%c", ndefBuff[i]);
                    }
                    printf("\r\n");
                    
                    printf("文本内容: ");
                    // 跳过状态字节(第1个payload字节)
                    for (i = 4 + typeLength; i < 4 + typeLength + payloadLength - 1; i++) {
                        printf("%c", ndefBuff[i]);
                    }
                    printf("\r\n");
                }
            }
        }
        
        free(ndefBuff);
        usleep(2000*1000); //2秒读取一次
    }
}

//NFC写入任务
osThreadId_t NFC_Write_Task_ID; //写入任务ID

// 使用真正的NT3H写入函数实现NFC数据写入
uint32_t NT3HWriteDataNfc(uint8_t *data, uint8_t length) {
    bool result = false;
    uint8_t page = 1; // 从第1页开始写入（第0页是NDEF头）
    uint8_t bytesWritten = 0;
    uint8_t pageBuffer[NFC_PAGE_SIZE];
    
    printf("开始写入NFC数据，长度: %d字节\n", length);
    
    // 首先写入NDEF消息头（第0页）
    result = NT3HWriteHeaderNfc(length, 0xD1);
    if (!result) {
        printf("写入NDEF头失败\n");
        return false;
    }
    printf("NDEF头写入成功\n");
    
    // 写入数据到用户内存区域
    while (bytesWritten < length) {
        // 清空页缓冲区
        memset(pageBuffer, 0, NFC_PAGE_SIZE);
        
        // 计算当前页要写入的字节数
        uint8_t bytesToWrite = (length - bytesWritten) > NFC_PAGE_SIZE ? NFC_PAGE_SIZE : (length - bytesWritten);
        
        // 复制数据到页缓冲区
        memcpy(pageBuffer, data + bytesWritten, bytesToWrite);
        
        // 写入当前页
        result = NT3HWriteUserData(page, pageBuffer);
        if (!result) {
            printf("写入第%d页失败\n", page);
            return false;
        }
        
        printf("第%d页写入成功，写入%d字节\n", page, bytesToWrite);
        bytesWritten += bytesToWrite;
        page++;
        
        // 检查是否超出用户内存范围
        if (page > (USER_END_REG - USER_START_REG)) {
            printf("超出用户内存范围\n");
            break;
        }
    }
    
    printf("NFC数据写入完成，总共写入%d字节\n", bytesWritten);
    return true;
}

void NFC_Write_Task(void) {
    uint32_t result_code = 0;
    uint8_t writeCount = 0;
    
    nfc_init();
    printf("NFC写入任务启动...\r\n");
    printf("使用真正的NT3H1101写入功能\r\n");

    while (1) 
    {
        // 等待5秒后开始写入
        usleep(5000*1000);
        
        // 创建要写入的文本数据
        char textData[100];
        snprintf(textData, sizeof(textData), "Hi3861 NFC Test Data %d", writeCount);
        
        // 构建NDEF文本记录
        uint8_t ndefRecord[100];
        uint8_t ndefLength = 0;
        
        // NDEF记录头: TNF=0x01(Well-Known), SR=1, IL=0, TYPE_LENGTH_FIELD=1
        ndefRecord[ndefLength++] = 0xD1; // MB=1, ME=1, CF=0, SR=1, IL=0, TNF=0x01
        
        // 载荷长度 (文本长度 + 状态字节)
        uint8_t payloadLength = strlen(textData) + 1;
        ndefRecord[ndefLength++] = payloadLength;
        
        // 类型长度
        ndefRecord[ndefLength++] = 0x01; // "T"
        
        // 类型
        ndefRecord[ndefLength++] = 'T'; // 文本记录类型
        
        // 状态字节: 语言编码长度=2 ("en")
        ndefRecord[ndefLength++] = 0x02; // 语言编码长度
        
        // 语言编码
        ndefRecord[ndefLength++] = 'e';
        ndefRecord[ndefLength++] = 'n';
        
        // 文本内容
        unsigned int i;
        for (i = 0; i < strlen(textData); i++) {
            ndefRecord[ndefLength++] = textData[i];
        }
        
        printf("准备写入数据: %s\r\n", textData);
        printf("NDEF记录长度: %d\r\n", ndefLength);
        
        // 真正的NFC写入功能
        if ((result_code = NT3HWriteDataNfc(ndefRecord, ndefLength)) == true) 
        {
            printf("NFC写入成功! 数据: %s\r\n", textData);
            writeCount++;
        }
        else 
        {
            printf("NFC写入失败! result_code = %d\r\n", result_code);
        }
        
        usleep(10000*1000); //10秒写入一次
    }
}

//NFC读取任务创建
void nfc_read_task_create(void)
{
    osThreadAttr_t taskOptions;
    taskOptions.name = "NFCReadTask";       // 任务的名字
    taskOptions.attr_bits = 0;               // 属性位
    taskOptions.cb_mem = NULL;               // 堆空间地址
    taskOptions.cb_size = 0;                 // 堆空间大小
    taskOptions.stack_mem = NULL;            // 栈空间地址
    taskOptions.stack_size = 1024*5;         // 栈空间大小 单位:字节
    taskOptions.priority = osPriorityNormal; // 任务的优先级

    NFC_Read_Task_ID = osThreadNew((osThreadFunc_t)NFC_Read_Task, NULL, &taskOptions); // 创建读取任务
    if (NFC_Read_Task_ID != NULL)
    {
        printf("ID = %d, NFC读取任务创建成功!\n", NFC_Read_Task_ID);
    }
}

//NFC写入任务创建
void nfc_write_task_create(void)
{
    osThreadAttr_t taskOptions;
    taskOptions.name = "NFCWriteTask";      // 任务的名字
    taskOptions.attr_bits = 0;               // 属性位
    taskOptions.cb_mem = NULL;               // 堆空间地址
    taskOptions.cb_size = 0;                 // 堆空间大小
    taskOptions.stack_mem = NULL;            // 栈空间地址
    taskOptions.stack_size = 1024*5;         // 栈空间大小 单位:字节
    taskOptions.priority = osPriorityNormal; // 任务的优先级

    NFC_Write_Task_ID = osThreadNew((osThreadFunc_t)NFC_Write_Task, NULL, &taskOptions); // 创建写入任务
    if (NFC_Write_Task_ID != NULL)
    {
        printf("ID = %d, NFC写入任务创建成功!\n", NFC_Write_Task_ID);
    }
}

/**
 * @description: 初始化并创建任务
 * @param {*}
 * @return {*}
 */
static void template_demo(void)
{
    printf("普中-Hi3861开发板--NFC实验(读取+写入)\r\n");
    led_task_create();
    nfc_read_task_create();  // 创建读取任务
    nfc_write_task_create(); // 创建写入任务
}
SYS_RUN(template_demo);