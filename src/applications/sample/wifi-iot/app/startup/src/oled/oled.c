#include "oled.h"
#include "oled_driver.h"
#include "songti_font.h"
#include <string.h>
#include <stdio.h>
#include "cmsis_os2.h"
#include "time.h"  // 添加时间库支持
#include <stdlib.h>  // 添加malloc/free支持

// 显示请求队列结构 - 优化内存使用
typedef struct {
    uint16_t x;
    uint16_t y;
    char text[64];  // 减少文本缓冲区大小
    uint8_t size;
    bool is_num;
    uint32_t num_value;
    uint8_t num_len;
} oled_request_t;

// 全局变量 - 改为堆内存分配
static osMessageQueueId_t g_oled_queue = NULL;
static osThreadId_t g_oled_task = NULL;
static bool g_oled_initialized = false;

// 简化显示函数（直接操作前缓冲区）
static void oled_show_string_direct(uint16_t x, uint16_t y, const char* str, uint8_t size)
{
    if (!oled_driver_is_initialized()) {
        return;
    }
    
    uint16_t x_pos = x;
    const char* str_ptr = str;
    while (*str_ptr) {
        const uint8_t* font_data = songti_font_get_char_data(*str_ptr);
        uint8_t font_width = songti_font_get_char_width(*str_ptr);
        uint8_t font_height = songti_font_get_char_height();
        
        for (uint8_t i = 0; i < font_height; i++) {
            uint8_t line_data = font_data[i];
            for (uint8_t j = 0; j < font_width; j++) {
                if (line_data & (1 << (7 - j))) {
                    if ((y + i) < OLED_HEIGHT) {
                        oled_driver_draw_pixel(x_pos + j, y + i, true);
                    }
                }
            }
        }
        
        x_pos += font_width + 1;
        str_ptr++;
    }
}

// OLED后台刷新任务（优化内存使用）
static void OLED_RefreshTask(void* arg)
{
    (void)arg;
    
    oled_request_t request;
    uint32_t last_refresh_time = Time_GetCurrentMs();
    const uint32_t REFRESH_INTERVAL_MS = 100; // 降低刷新频率为200ms
    
    while (1) {
        uint32_t current_time = Time_GetCurrentMs();
        
        // 处理显示请求（简化处理逻辑）
        if (osMessageQueueGet(g_oled_queue, &request, NULL, 0) == osOK) {
            // 直接在前缓冲区绘制，减少内存使用
            if (request.is_num) {
                char num_str[12];
                snprintf(num_str, sizeof(num_str), "%*lu", request.num_len, request.num_value);
                oled_show_string_direct(request.x, request.y, num_str, request.size);
            } else {
                oled_show_string_direct(request.x, request.y, request.text, request.size);
            }
            
            // 立即刷新修改的页面
            oled_driver_refresh_partial();
        }
        
        // 定期完整刷新（降低频率）
        if (current_time - last_refresh_time >= REFRESH_INTERVAL_MS) {
            oled_driver_refresh();
            last_refresh_time = current_time;
        }
        
        // 增加延时，减少CPU占用
        Time_DelayMs(10);
    }
}

// OLED初始化（优化内存使用）- 修正返回类型为int
int OLED_Init(void)
{
    if (g_oled_initialized) {
        return 0;
    }
    
    // 初始化OLED驱动器（使用堆内存）
    if (oled_driver_init() != 0) {
        printf("OLED: Failed to initialize driver!\n");
        return -1;
    }
    
    // 创建消息队列（减少队列大小）
    osMessageQueueAttr_t queue_attr = {
        .name = "OLEDQueue",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U,
        .mq_mem = NULL,
        .mq_size = 0U
    };
    g_oled_queue = osMessageQueueNew(5, sizeof(oled_request_t), &queue_attr); // 减少队列大小
    
    if (g_oled_queue == NULL) {
        printf("OLED: Failed to create message queue!\n");
        oled_driver_deinit();
        return -1;
    }
    
    // 创建OLED后台任务（减少栈大小）
    osThreadAttr_t task_attr = {
        .name = "OLEDTask",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U,
        .stack_mem = NULL,
        .stack_size = 1024, // 减少栈大小
        .priority = osPriorityBelowNormal // 降低优先级，避免影响传感器
    };
    
    g_oled_task = osThreadNew(OLED_RefreshTask, NULL, &task_attr);
    if (g_oled_task == NULL) {
        printf("OLED: Failed to create refresh task!\n");
        osMessageQueueDelete(g_oled_queue);
        oled_driver_deinit();
        return -1;
    }
    
    g_oled_initialized = true;
    printf("OLED initialized with heap memory! (Reduced resource usage)\n");
    return 0;
}

void OLED_Deinit(void)
{
    if (!g_oled_initialized) {
        return;
    }
    
    if (g_oled_task) {
        osThreadTerminate(g_oled_task);
        g_oled_task = NULL;
    }
    
    if (g_oled_queue) {
        osMessageQueueDelete(g_oled_queue);
        g_oled_queue = NULL;
    }
    
    oled_driver_deinit();
    g_oled_initialized = false;
}

void OLED_Clear(void)
{
    oled_driver_clear();
}

void OLED_Refresh(void)
{
    if (g_oled_initialized) {
        oled_driver_refresh();
    }
}

// 直接显示字符串（阻塞式）
void OLED_ShowString(uint16_t x, uint16_t y, const char* str, uint8_t size)
{
    if (!g_oled_initialized) {
        return;
    }
    
    uint16_t x_pos = x;
    const char* str_ptr = str;
    while (*str_ptr) {
        const uint8_t* font_data = songti_font_get_char_data(*str_ptr);
        uint8_t font_width = songti_font_get_char_width(*str_ptr);
        uint8_t font_height = songti_font_get_char_height();
        
        for (uint8_t i = 0; i < font_height; i++) {
            uint8_t line_data = font_data[i];
            for (uint8_t j = 0; j < font_width; j++) {
                if (line_data & (1 << (7 - j))) {
                    if ((y + i) < OLED_HEIGHT) {
                        oled_driver_draw_pixel(x_pos + j, y + i, true);
                    }
                }
            }
        }
        
        x_pos += font_width + 1;
        str_ptr++;
    }
}

// 直接显示数字（阻塞式）
void OLED_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size)
{
    char num_str[12];
    snprintf(num_str, sizeof(num_str), "%*lu", len, num);
    OLED_ShowString(x, y, num_str, size);
}

// 直接显示温湿度数据（新增）
void OLED_ShowSensorData(const oled_sensor_data_t* sensor_data)
{
    if (!g_oled_initialized || sensor_data == NULL) {
        return;
    }
    
    OLED_Clear();
    
    char temp_str[32];
    char hum_str[32];
    char smoke_str[32];
    char status_str[32];
    
    // 显示温度
    snprintf(temp_str, sizeof(temp_str), "Temp: %.1fC", sensor_data->temperature);
    OLED_ShowString(0, 0, temp_str, 8);
    
    // 显示湿度
    snprintf(hum_str, sizeof(hum_str), "Humidity: %d%%", sensor_data->humidity);
    OLED_ShowString(0, 10, hum_str, 8);
    
    // 显示烟雾值
    snprintf(smoke_str, sizeof(smoke_str), "Smoke: %d", sensor_data->smoke_value);
    OLED_ShowString(0, 20, smoke_str, 8);
    
    // 显示当前状态
    if (!sensor_data->alarm_active) {
        snprintf(status_str, sizeof(status_str), "Status: Normal");
    } else {
        if (sensor_data->alarm_level == 1) {
            snprintf(status_str, sizeof(status_str), "Status: Yellow Alarm");
        } else if (sensor_data->alarm_level == 2) {
            snprintf(status_str, sizeof(status_str), "Status: Red Alarm");
        } else {
            snprintf(status_str, sizeof(status_str), "Status: Unknown");
        }
    }
    OLED_ShowString(0, 30, status_str, 8);
    
    // 刷新显示
    OLED_Refresh();
}

// 请求显示字符串（非阻塞）
void OLED_RequestShowString(uint16_t x, uint16_t y, const char* str, uint8_t size)
{
    if (!g_oled_initialized || !g_oled_queue) {
        return;
    }
    
    oled_request_t request = {
        .x = x,
        .y = y,
        .size = size,
        .is_num = false
    };
    
    // 复制字符串（限制长度避免溢出）
    strncpy(request.text, str, sizeof(request.text) - 1);
    request.text[sizeof(request.text) - 1] = '\0';
    
    // 发送请求（非阻塞）
    osMessageQueuePut(g_oled_queue, &request, 0, 0);
}

// 请求显示数字（非阻塞）
void OLED_RequestShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size)
{
    if (!g_oled_initialized || !g_oled_queue) {
        return;
    }
    
    oled_request_t request = {
        .x = x,
        .y = y,
        .size = size,
        .is_num = true,
        .num_value = num,
        .num_len = len
    };
    
    // 发送请求（非阻塞）
    osMessageQueuePut(g_oled_queue, &request, 0, 0);
}

void OLED_ForceRefresh(void)
{
    if (g_oled_initialized) {
        oled_driver_refresh_fast();
    }
}

bool OLED_IsReady(void)
{
    return g_oled_initialized;
}