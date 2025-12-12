#include "oled.h"
#include "oled_driver.h"
#include "songti_font.h"
#include <string.h>
#include <stdio.h>
#include "cmsis_os2.h"

// 显示请求队列结构
typedef struct {
    uint16_t x;
    uint16_t y;
    char text[32];
    uint8_t size;
    bool is_num;
    uint32_t num_value;
    uint8_t num_len;
} oled_request_t;

// 全局变量
static osMessageQueueId_t g_oled_queue = NULL;
static osThreadId_t g_oled_task = NULL;
static bool g_oled_initialized = false;

// OLED后台刷新任务（修复版本）
static void OLED_RefreshTask(void* arg)
{
    (void)arg;
    
    oled_request_t request;
    osStatus_t status;
    uint32_t last_refresh_time = osKernelGetTickCount();
    const uint32_t REFRESH_INTERVAL = 10; // 10ms刷新间隔
    bool need_refresh = false;
    
    // 初始化后缓冲区
    oled_driver_clear_backbuffer();
    
    // 初始显示内容
    OLED_RequestShowString(0, 0, "System Ready", 8);
    
    while (1) {
        uint32_t current_time = osKernelGetTickCount();
        
        // 处理所有待处理的显示请求
        while (osMessageQueueGet(g_oled_queue, &request, NULL, 0) == osOK) {
            // 在后缓冲区显示内容
            if (request.is_num) {
                char num_str[20];
                snprintf(num_str, sizeof(num_str), "%*lu", request.num_len, request.num_value);
                
                // 在后缓冲区显示数字
                uint16_t x_pos = request.x;
                const char* str_ptr = num_str;
                while (*str_ptr) {
                    const uint8_t* font_data = songti_font_get_char_data(*str_ptr);
                    uint8_t font_width = songti_font_get_char_width(*str_ptr);
                    uint8_t font_height = songti_font_get_char_height();
                    
                    for (uint8_t i = 0; i < font_height; i++) {
                        uint8_t line_data = font_data[i];
                        for (uint8_t j = 0; j < font_width; j++) {
                            if (line_data & (1 << (7 - j))) {
                                if ((request.y + i) < OLED_HEIGHT) {
                                    oled_driver_draw_pixel_backbuffer(x_pos + j, request.y + i, true);
                                }
                            }
                        }
                    }
                    
                    x_pos += font_width + 1;
                    str_ptr++;
                }
            } else {
                // 在后缓冲区显示字符串
                uint16_t x_pos = request.x;
                const char* str_ptr = request.text;
                while (*str_ptr) {
                    const uint8_t* font_data = songti_font_get_char_data(*str_ptr);
                    uint8_t font_width = songti_font_get_char_width(*str_ptr);
                    uint8_t font_height = songti_font_get_char_height();
                    
                    for (uint8_t i = 0; i < font_height; i++) {
                        uint8_t line_data = font_data[i];
                        for (uint8_t j = 0; j < font_width; j++) {
                            if (line_data & (1 << (7 - j))) {
                                if ((request.y + i) < OLED_HEIGHT) {
                                    oled_driver_draw_pixel_backbuffer(x_pos + j, request.y + i, true);
                                }
                            }
                        }
                    }
                    
                    x_pos += font_width + 1;
                    str_ptr++;
                }
            }
            
            need_refresh = true;
        }
        
        // 统一刷新时机：定期刷新或需要刷新时
        if (need_refresh || (current_time - last_refresh_time >= REFRESH_INTERVAL)) {
            // 交换缓冲区并刷新
            oled_driver_swap_buffers();
            oled_driver_refresh_fast();
            
            last_refresh_time = current_time;
            need_refresh = false;
        }
        
        // 短延时，让出CPU给其他任务
        osDelay(5); // 只阻塞5ms
    }
}

// OLED初始化（包含后台任务）
void OLED_Init(void)
{
    if (g_oled_initialized) {
        return;
    }
    
    // 初始化OLED驱动器
    oled_driver_init();
    
    // 创建消息队列
    osMessageQueueAttr_t queue_attr = {
        .name = "OLEDQueue",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U,
        .mq_mem = NULL,
        .mq_size = 0U
    };
    g_oled_queue = osMessageQueueNew(10, sizeof(oled_request_t), &queue_attr);
    
    if (g_oled_queue == NULL) {
        printf("Failed to create OLED message queue!\n");
        return;
    }
    
    // 创建OLED后台任务
    osThreadAttr_t task_attr = {
        .name = "OLEDTask",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U,
        .stack_mem = 2048,
        .stack_size = 2048,
        .priority = osPriorityNormal
    };
    
    g_oled_task = osThreadNew(OLED_RefreshTask, NULL, &task_attr);
    if (g_oled_task == NULL) {
        printf("Failed to create OLED refresh task!\n");
        osMessageQueueDelete(g_oled_queue);
        return;
    }
    
    // 修复：初始化后立即显示一些内容，确保屏幕亮起
    oled_driver_set_display(true); // 确保显示开启
    oled_driver_refresh(); // 立即刷新一次
    
    g_oled_initialized = true;
    printf("OLED initialized with background refresh task!\n");
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