#include "oled.h"
#include "oled_driver.h"
#include "songti_font.h"
#include <string.h>
#include <stdio.h>
#include "cmsis_os2.h"
#include "time.h"
#include <stdlib.h>

// ======================== 文本缓冲区定义 ========================
#define OLED_BUF_MAX_LINES  16      // 最大行数
#define OLED_BUF_MAX_CHARS  64      // 每行最大字符数

typedef struct {
    char text[OLED_BUF_MAX_CHARS];
    uint8_t font_size;     // 目标字符宽度（像素）
    bool in_use;
    bool terminated;       // true=由\n结束（硬换行），false=自动换行/末尾（软换行，后续可继续追加）
} oled_buf_line_t;

static oled_buf_line_t g_oled_buf[OLED_BUF_MAX_LINES];
static bool g_oled_buf_dirty = false;
static bool g_oled_clear_requested = false; // 用户显式调用 OLED_Clear 的标记

// ======================== 静态工具函数 ========================

// 计算指定字号的行高（像素）
static uint8_t oled_get_line_height(uint8_t font_size)
{
    uint8_t fw = songti_font_get_char_width(' ');
    uint8_t fh = songti_font_get_char_height();
    return (uint8_t)((uint32_t)font_size * fh / fw);
}

// 向缓冲区添加一行（自动滚动）
// terminated: true=由\n结束，false=自动换行/末尾
static void oled_buf_add_line(const char* text, uint8_t font_size, bool terminated)
{
    // 缓冲区满则上滚（丢弃最旧的一行）
    if (g_oled_buf[OLED_BUF_MAX_LINES - 1].in_use) {
        for (int i = 0; i < OLED_BUF_MAX_LINES - 1; i++) {
            g_oled_buf[i] = g_oled_buf[i + 1];
        }
        g_oled_buf[OLED_BUF_MAX_LINES - 1].in_use = false;
    }

    // 找空位
    int slot = -1;
    for (int i = 0; i < OLED_BUF_MAX_LINES; i++) {
        if (!g_oled_buf[i].in_use) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return;

    strncpy(g_oled_buf[slot].text, text, OLED_BUF_MAX_CHARS - 1);
    g_oled_buf[slot].text[OLED_BUF_MAX_CHARS - 1] = '\0';
    g_oled_buf[slot].font_size = font_size;
    g_oled_buf[slot].in_use = true;
    g_oled_buf[slot].terminated = terminated;
}

// 前向声明
static void oled_show_string_direct(uint16_t x, int y, const char* str, uint8_t size);

// 从底部倒推，计算屏幕能显示的第一行（自动滚屏）
// out_y_offset: 输出参数，第一行渲染的 Y 坐标（确保最后一行紧贴屏幕底部）
// 返回：从此行开始渲染，之前的旧行被滚出屏幕
static int oled_get_scroll_start(int *out_y_offset)
{
    int last_line = -1;
    for (int i = OLED_BUF_MAX_LINES - 1; i >= 0; i--) {
        if (g_oled_buf[i].in_use) {
            last_line = i;
            break;
        }
    }
    if (last_line < 0) {
        *out_y_offset = 0;
        return 0;
    }

    int h = 0;       // 可见行总高度
    int start = 0;

    for (int i = last_line; i >= 0; i--) {
        if (!g_oled_buf[i].in_use) continue;

        int line_h = (int)oled_get_line_height(g_oled_buf[i].font_size);
        int spacing = (h > 0) ? 1 : 0;  // 非最底行时加行间距

        if (h + spacing + line_h > OLED_HEIGHT) {
            // 屏幕放不下了，但可以让当前行露出下半部分
            // （滚动时顶部不整行消失，过渡更自然）
            int remaining = OLED_HEIGHT - h;
            if (remaining > spacing) {
                // 至少还有间距+部分文字能显示
                h += spacing + line_h;  // 计入完整行高，h > OLED_HEIGHT，y_off 为负
                start = i;
            }
            break;
        }

        h += spacing + line_h;
        start = i;
    }

    // 当无溢出时 y_off > 0（内容不满屏），有溢出时 y_off ≤ 0（贴底或顶部露半截）
    *out_y_offset = OLED_HEIGHT - h;
    return start;
}

// 渲染缓冲区内容到屏幕（带滚屏，最后一行紧贴屏幕底部）
static void oled_render_buffer(void)
{
    // 先检查是否有缓冲区内容需要渲染
    bool has_content = false;
    for (int i = 0; i < OLED_BUF_MAX_LINES; i++) {
        if (g_oled_buf[i].in_use) {
            has_content = true;
            break;
        }
    }

    if (!has_content) {
        if (g_oled_clear_requested) {
            // 用户调用了 OLED_Clear，需要物理清屏
            oled_driver_clear();
            g_oled_clear_requested = false;
            oled_driver_refresh_fast();
        }
        // 缓冲区无内容且无清屏请求 → 不动屏幕（保护已显示的图片）
        return;
    }

    // 有文本内容 → 双缓冲绘制
    g_oled_clear_requested = false;

    // begin_render: gram 重定向到后缓冲区并清空，所有绘制写入不可见区域
    oled_driver_begin_render();

    int y_off;
    int start = oled_get_scroll_start(&y_off);
    int y = (start > 0 || y_off < 0) ? y_off : 0;
    for (int i = start; i < OLED_BUF_MAX_LINES && y < OLED_HEIGHT; i++) {
        if (g_oled_buf[i].in_use) {
            oled_show_string_direct(0, y, g_oled_buf[i].text, g_oled_buf[i].font_size);
            y += oled_get_line_height(g_oled_buf[i].font_size) + 1;
        }
    }

    // end_render: 恢复 gram，交换缓冲区（O(1)指针交换），刷新屏幕
    oled_driver_end_render();
}

// 简化显示函数（批量显存写入，像素级最近邻缩放）
// y 支持负值：文字上半部分被裁切，下半部分可见（用于滚动时顶部露出半截）
static void oled_show_string_direct(uint16_t x, int y, const char* str, uint8_t size)
{
    if (!oled_driver_is_initialized()) {
        return;
    }
    
    const char* str_ptr = str;
    uint8_t font_width = songti_font_get_char_width(*str_ptr);
    uint8_t font_height = songti_font_get_char_height();
    uint8_t target_width = (size < 1) ? font_width : size;
    uint8_t target_height = (uint8_t)((uint32_t)target_width * font_height / font_width);
    if (y >= (int)OLED_HEIGHT) return;
    if (y + (int)target_height <= 0) return;
    
    uint16_t x_pos = x;
    uint16_t w_bytes = (font_width + 7) / 8;
    
    // ========== 无缩放快速路径：批量写显存（替代像素级逐点绘制） ==========
    if (size < 1 || size == font_width) {
        while (*str_ptr) {
            const uint8_t* font_data = songti_font_get_char_data(*str_ptr);
            
            if (y >= 0) {
                // 完全可见：一次批量写入替代 font_width*font_height 次 draw_pixel
                oled_driver_write_gram_block_from_bitmap(
                    x_pos, (uint16_t)y, font_width, font_height, font_data);
            } else {
                // 顶部部分裁切（滚动时最上一行露出半截）
                uint16_t skip_rows = (uint16_t)(-y);
                if (skip_rows < font_height) {
                    oled_driver_write_gram_block_from_bitmap(
                        x_pos, 0, font_width, font_height - skip_rows,
                        font_data + skip_rows * w_bytes);
                }
            }
            
            x_pos += target_width + 1;
            str_ptr++;
        }
        return;
    }
    
    // ========== 缩放路径：像素级渲染 ==========
    uint32_t step_x = ((uint32_t)font_width << 16) / target_width;
    uint32_t step_y = ((uint32_t)font_height << 16) / target_height;
    
    while (*str_ptr) {
        const uint8_t* font_data = songti_font_get_char_data(*str_ptr);
        
        for (uint16_t ty = 0; ty < target_height; ty++) {
            int py = y + (int)ty;
            if (py < 0 || py >= (int)OLED_HEIGHT) continue;
            
            uint8_t src_y = (uint8_t)((ty * step_y) >> 16);
            if (src_y >= font_height) src_y = font_height - 1;
            uint8_t line_data = font_data[src_y];
            
            for (uint16_t tx = 0; tx < target_width; tx++) {
                uint8_t src_x = (uint8_t)((tx * step_x) >> 16);
                if (src_x >= font_width) src_x = font_width - 1;
                
                if (line_data & (1 << (7 - src_x))) {
                    uint16_t px = x_pos + tx;
                    if (px < OLED_WIDTH) {
                        oled_driver_draw_pixel(px, (uint16_t)py, true);
                    }
                }
            }
        }
        
        x_pos += target_width + 1;
        str_ptr++;
    }
}

// ======================== 全局变量 ========================
static osThreadId_t g_oled_task = NULL;
static bool g_oled_initialized = false;
static oled_refresh_mode_t g_refresh_mode = OLED_REFRESH_POLLING;
static osSemaphoreId_t g_oled_sem = NULL;   // 事件模式信号量

// 通知后台任务刷新（事件模式使用信号量唤醒）
static inline void oled_signal_refresh(void)
{
    g_oled_buf_dirty = true;
    if (g_oled_task && g_refresh_mode == OLED_REFRESH_EVENT && g_oled_sem) {
        osSemaphoreRelease(g_oled_sem);
    }
}

// ======================== 后台刷新任务 ========================
// 支持两种唤醒策略（通过 g_refresh_mode 切换，无需两套独立 task）：
//   EVENT  — 信号量唤醒，无更新时任务完全挂起（零 CPU）
//   POLLING — 每 10ms 检查脏标记（简单可靠）
static void OLED_RefreshTask(void* arg)
{
    (void)arg;

    while (1) {
        if (g_refresh_mode == OLED_REFRESH_EVENT) {
            // 事件模式：阻塞等信号量（无更新时任务挂起，CPU 归零）
            if (g_oled_sem) {
                osSemaphoreAcquire(g_oled_sem, osWaitForever);
            }
        }

        // Drain 循环：一次处理完所有待刷新内容
        // 快速连续 Printf 会被合并为一次渲染，避免闪烁
        while (g_oled_buf_dirty) {
            g_oled_buf_dirty = false;
            oled_render_buffer();
        }

        if (g_refresh_mode == OLED_REFRESH_POLLING) {
            // 轮询模式：固定间隔检查
            Time_DelayMs(10);
        }
    }
}

// ======================== 初始化/反初始化 ========================

int OLED_Init(oled_refresh_mode_t mode)
{
    if (g_oled_initialized) return 0;

    if (oled_driver_init() != 0) {
        printf("OLED: Failed to initialize driver!\n");
        return -1;
    }

    g_refresh_mode = mode;

    // 事件模式需要创建信号量
    if (mode == OLED_REFRESH_EVENT) {
        g_oled_sem = osSemaphoreNew(1, 0, NULL);  // 二值信号量，初始为 0
        if (g_oled_sem == NULL) {
            printf("OLED: Failed to create semaphore!\n");
            oled_driver_deinit();
            return -1;
        }
    }

    // 清空缓冲区
    memset(g_oled_buf, 0, sizeof(g_oled_buf));

    // 创建后台刷新任务（统一 task，内部根据 g_refresh_mode 切换唤醒策略）
    osThreadAttr_t task_attr = {
        .name = "OLEDTask",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U,
        .stack_mem = NULL,
        .stack_size = 1024,
        .priority = osPriorityNormal
    };

    g_oled_task = osThreadNew(OLED_RefreshTask, NULL, &task_attr);
    if (g_oled_task == NULL) {
        printf("OLED: Failed to create refresh task!\n");
        oled_driver_deinit();
        return -1;
    }

    g_oled_initialized = true;
    g_oled_buf_dirty = false;
    printf("OLED initialized (mode: %s)!\n", mode == OLED_REFRESH_EVENT ? "event" : "polling");
    return 0;
}

void OLED_Deinit(void)
{
    if (!g_oled_initialized) return;

    if (g_oled_task) {
        osThreadTerminate(g_oled_task);
        g_oled_task = NULL;
    }

    if (g_oled_sem) {
        osSemaphoreDelete(g_oled_sem);
        g_oled_sem = NULL;
    }

    oled_driver_deinit();
    g_oled_initialized = false;
}

// ======================== 缓冲区操作 ========================

// Tab 宽度（按字符数计算）
#define OLED_TAB_WIDTH 4

void OLED_Clear(void)
{
    // 清空缓冲区（不直接刷新屏幕，让后台任务统一处理）
    for (int i = 0; i < OLED_BUF_MAX_LINES; i++) {
        g_oled_buf[i].in_use = false;
    }
    g_oled_clear_requested = true;  // 标记需要物理清屏
    oled_signal_refresh();
}

void OLED_Refresh(void)
{
    if (!g_oled_initialized) return;

    // 手动触发缓冲区渲染
    oled_signal_refresh();
}

bool OLED_IsReady(void)
{
    return g_oled_initialized;
}

// 翻转显示颜色（白底黑字 ↔ 黑底白字）
void OLED_Invert(bool invert)
{
    if (!g_oled_initialized) return;
    oled_driver_set_display_mode(invert ? OLED_DISPLAY_INVERTED : OLED_DISPLAY_NORMAL);
}

// 设置对比度（0~255，越大越亮）
void OLED_SetContrast(uint8_t contrast)
{
    if (!g_oled_initialized) return;
    oled_driver_set_contrast(contrast);
}

// 控制屏幕开关（不断电，仅关闭/开启显示）
void OLED_DisplayOn(bool on)
{
    if (!g_oled_initialized) return;
    oled_driver_set_display(on);
}

// 等待所有待刷新的 Printf 渲染完成（同步屏障）
void OLED_Flush(void)
{
    if (!g_oled_initialized) return;

    if (g_refresh_mode == OLED_REFRESH_EVENT) {
        // 事件模式：同步渲染（flush 本意就是确保屏幕已更新）
        // 在当前上下文直接执行渲染，包含完整 I2C 传输时间
        if (g_oled_buf_dirty) {
            g_oled_buf_dirty = false;
            oled_render_buffer();
        }
        // Drain 剩余（可能有并发 Printf 设了脏标记）
        while (g_oled_buf_dirty) {
            g_oled_buf_dirty = false;
            oled_render_buffer();
        }
    } else {
        // 轮询模式：渲染任务每 10ms 才检查脏标记，必须等一轮 tick
        for (int retry = 0; retry < 5; retry++) {
            if (!g_oled_buf_dirty) break;
            osDelay(1);
        }
    }
}

// ======================== 图片显示 ========================

// 显示图片（页格式数据，直接写显存，最快）
void OLED_ShowImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data)
{
    if (!g_oled_initialized || data == NULL) return;
    oled_driver_begin_render();
    oled_driver_write_gram_block(x, y, w, h, data);
    oled_driver_end_render();
}

// 显示水平 bitmap（行优先格式，自动转换为页格式）
// 兼容 "128x64 mono bitmap" 最常见格式
void OLED_ShowBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *bitmap)
{
    if (!g_oled_initialized || bitmap == NULL) return;
    oled_driver_begin_render();
    oled_driver_write_gram_block_from_bitmap(x, y, w, h, bitmap);
    oled_driver_end_render();
}

// 直接叠印文本到屏幕（不经过缓冲区，不擦除已有内容）
// 用于在图片/画面上实时叠加文字（如 FPS）
void OLED_DrawText(uint16_t x, uint16_t y, uint8_t size, const char* str)
{
    if (!g_oled_initialized || str == NULL) return;
    oled_driver_begin_render_copy();
    oled_show_string_direct(x, (int)y, str, size);
    oled_driver_end_render();
}

// ======================== Printf 缓冲区显示 ========================

void OLED_Printf(uint8_t size, const char* fmt, ...)
{
    if (!g_oled_initialized) return;

    char buf[OLED_BUF_MAX_CHARS];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    // 计算每行最多容纳字符数
    uint8_t char_pitch = size + 1;          // 每个字符占的宽度（字宽+间距）
    uint8_t max_chars = OLED_WIDTH / char_pitch;
    if (max_chars < 1) max_chars = 1;

    // 按 \n 和自动换行分割，逐行加入缓冲区
    char segment[OLED_BUF_MAX_CHARS];
    int seg_len = 0;
    uint8_t actual_size = size;

    // ======== 尝试从缓冲区最后一行继续追加 ========
    // 只有字号一致且是软换行时，才接着上一行继续写
    int last_idx = -1;
    for (int i = OLED_BUF_MAX_LINES - 1; i >= 0; i--) {
        if (g_oled_buf[i].in_use) {
            last_idx = i;
            break;
        }
    }
    if (last_idx >= 0 && !g_oled_buf[last_idx].terminated 
        && g_oled_buf[last_idx].font_size == size) {
        // 字号一致 + 软换行 → 继续追加
        strncpy(segment, g_oled_buf[last_idx].text, OLED_BUF_MAX_CHARS - 1);
        segment[OLED_BUF_MAX_CHARS - 1] = '\0';
        seg_len = (int)strlen(segment);
        actual_size = size;  // 保持用当前传入的字号
        g_oled_buf[last_idx].in_use = false;
    }

    char* p = buf;

    while (*p) {
        if (*p == '\t') {
            // Tab 展开：补空格到下一个 Tab 位
            int next_tab = (seg_len / OLED_TAB_WIDTH + 1) * OLED_TAB_WIDTH;
            int spaces = next_tab - seg_len;
            for (int i = 0; i < spaces; i++) {
                // 当前行已满 → 先换行（软换行）
                if (seg_len >= (int)max_chars) {
                    segment[seg_len] = '\0';
                    oled_buf_add_line(segment, actual_size, false);
                    seg_len = 0;
                }
                if (seg_len >= OLED_BUF_MAX_CHARS - 1) {
                    segment[seg_len] = '\0';
                    oled_buf_add_line(segment, actual_size, false);
                    seg_len = 0;
                }
                segment[seg_len++] = ' ';
            }
            p++;
            continue;
        }

        if (*p == '\n') {
            // \n 换行：把当前积累的文本作为一行（硬换行）
            if (seg_len > 0) {
                segment[seg_len] = '\0';
                oled_buf_add_line(segment, actual_size, true);
                seg_len = 0;
            } else {
                // 连续 \n 或开头 \n → 加空行（硬换行）
                oled_buf_add_line("", actual_size, true);
            }
            p++;
            continue;
        }

        // 当前行已满 → 先换行，再添加到新行（软换行，后续可继续追加）
        if (seg_len >= (int)max_chars) {
            segment[seg_len] = '\0';
            oled_buf_add_line(segment, actual_size, false);
            seg_len = 0;
        }

        // 缓冲区上限保护（极长行截断，软换行）
        if (seg_len >= OLED_BUF_MAX_CHARS - 1) {
            segment[seg_len] = '\0';
            oled_buf_add_line(segment, actual_size, false);
            seg_len = 0;
        }

        segment[seg_len++] = *p;
        p++;
    }

    // 剩余文本（软换行，后续 Printf 可继续追加）
    if (seg_len > 0) {
        segment[seg_len] = '\0';
        oled_buf_add_line(segment, actual_size, false);
    }

    // 标记脏，唤醒后台任务渲染
    oled_signal_refresh();
}