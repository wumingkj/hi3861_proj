#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include "time.h"

// 调试级别定义
typedef enum {
    DEBUG_LEVEL_NONE = 0,    // 无调试信息
    DEBUG_LEVEL_ERROR,       // 错误级别
    DEBUG_LEVEL_WARN,        // 警告级别
    DEBUG_LEVEL_INFO,        // 信息级别
    DEBUG_LEVEL_DEBUG,       // 调试级别
    DEBUG_LEVEL_VERBOSE      // 详细级别
} debug_level_t;

// 全局调试级别设置（默认显示错误、警告和信息级别）
#ifndef DEFAULT_DEBUG_LEVEL
#define DEFAULT_DEBUG_LEVEL DEBUG_LEVEL_INFO
#endif

// 调试颜色定义（如果终端支持颜色）
#ifdef DEBUG_ENABLE_COLOR
    #define COLOR_RED     "\033[31m"
    #define COLOR_GREEN   "\033[32m"
    #define COLOR_YELLOW  "\033[33m"
    #define COLOR_BLUE    "\033[34m"
    #define COLOR_CYAN    "\033[36m"
    #define COLOR_RESET   "\033[0m"
#else
    #define COLOR_RED     ""
    #define COLOR_GREEN   ""
    #define COLOR_YELLOW  ""
    #define COLOR_BLUE    ""
    #define COLOR_CYAN    ""
    #define COLOR_RESET   ""
#endif

// 调试级别前缀
#define DEBUG_PREFIX_ERROR   COLOR_RED    "[E] "
#define DEBUG_PREFIX_WARN    COLOR_YELLOW "[W] "
#define DEBUG_PREFIX_INFO    COLOR_GREEN  "[I] "
#define DEBUG_PREFIX_DEBUG   COLOR_BLUE   "[D] "
#define DEBUG_PREFIX_VERBOSE COLOR_CYAN   "[V] "

// 调试宏核心函数
static inline int debug_print(debug_level_t level, const char* tag, const char* format, ...) {
    // 检查调试级别是否启用
    if (level > DEFAULT_DEBUG_LEVEL) {
        return 0;
    }
    
    // 获取当前时间
    uint32_t current_time = Time_GetCurrentMs();
    
    // 根据级别选择前缀
    const char* prefix = "";
    switch (level) {
        case DEBUG_LEVEL_ERROR:  prefix = DEBUG_PREFIX_ERROR; break;
        case DEBUG_LEVEL_WARN:   prefix = DEBUG_PREFIX_WARN; break;
        case DEBUG_LEVEL_INFO:   prefix = DEBUG_PREFIX_INFO; break;
        case DEBUG_LEVEL_DEBUG:  prefix = DEBUG_PREFIX_DEBUG; break;
        case DEBUG_LEVEL_VERBOSE:prefix = DEBUG_PREFIX_VERBOSE; break;
        default: return 0;
    }
    
    // 打印时间戳和前缀
    printf("[%8lu] %s", current_time, prefix);
    
    // 打印标签（如果提供）
    if (tag != NULL && strlen(tag) > 0) {
        printf("[%s] ", tag);
    }
    
    // 打印实际消息
    va_list args;
    va_start(args, format);
    int result = vprintf(format, args);
    va_end(args);
    
    // 重置颜色并换行
    printf("%s\n", COLOR_RESET);
    
    return result;
}

// 通用调试宏定义
#define log_e(tag, format, ...) debug_print(DEBUG_LEVEL_ERROR, tag, format, ##__VA_ARGS__)
#define log_w(tag, format, ...) debug_print(DEBUG_LEVEL_WARN, tag, format, ##__VA_ARGS__)
#define log_i(tag, format, ...) debug_print(DEBUG_LEVEL_INFO, tag, format, ##__VA_ARGS__)
#define log_d(tag, format, ...) debug_print(DEBUG_LEVEL_DEBUG, tag, format, ##__VA_ARGS__)
#define log_v(tag, format, ...) debug_print(DEBUG_LEVEL_VERBOSE, tag, format, ##__VA_ARGS__)

// 条件调试宏
#define log_if(condition, level, tag, format, ...) \
    do { \
        if (condition) { \
            debug_print(level, tag, format, ##__VA_ARGS__); \
        } \
    } while(0)

// 断言宏
#define debug_assert(condition, format, ...) \
    do { \
        if (!(condition)) { \
            log_e("ASSERT", "Assertion failed at %s:%d", __FILE__, __LINE__); \
            log_e("ASSERT", format, ##__VA_ARGS__); \
            while(1) { /* 挂起系统 */ } \
        } \
    } while(0)

#endif // _DEBUG_H_