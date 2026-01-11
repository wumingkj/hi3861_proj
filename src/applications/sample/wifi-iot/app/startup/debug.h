#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// 字符编码设置 - 解决中文乱码问题（避免与charset_utils.h冲突）
#ifndef DEBUG_CHARSET_UTF8
#define DEBUG_CHARSET_UTF8 1
#endif

#ifndef DEBUG_CHARSET_GBK
#define DEBUG_CHARSET_GBK 2
#endif

// 默认使用UTF-8编码（推荐）
#ifndef DEFAULT_CHARSET
#define DEFAULT_CHARSET DEBUG_CHARSET_UTF8
#endif

// 调试级别定义
typedef enum {
    DEBUG_LEVEL_NONE = 0,  // 无调试信息
    DEBUG_LEVEL_ERROR,     // 错误级别
    DEBUG_LEVEL_WARN,      // 警告级别
    DEBUG_LEVEL_INFO,      // 信息级别
    DEBUG_LEVEL_DEBUG,     // 调试级别
    DEBUG_LEVEL_VERBOSE    // 详细级别
} debug_level_t;

// 全局调试级别设置（默认显示错误、警告和信息级别）
#ifndef DEFAULT_DEBUG_LEVEL
#define DEFAULT_DEBUG_LEVEL DEBUG_LEVEL_INFO
#define DEBUG_ENABLE_COLOR
#endif

// 调试颜色定义（如果终端支持颜色）
#ifdef DEBUG_ENABLE_COLOR
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_CYAN "\033[36m"
#define COLOR_RESET "\033[0m"
#else
#define COLOR_RED ""
#define COLOR_GREEN ""
#define COLOR_YELLOW ""
#define COLOR_BLUE ""
#define COLOR_CYAN ""
#define COLOR_RESET ""
#endif

// 调试级别前缀
#define DEBUG_PREFIX_ERROR COLOR_RED "[E] "
#define DEBUG_PREFIX_WARN COLOR_YELLOW "[W] "
#define DEBUG_PREFIX_INFO COLOR_GREEN "[I] "
#define DEBUG_PREFIX_DEBUG COLOR_BLUE "[D] "
#define DEBUG_PREFIX_VERBOSE COLOR_CYAN "[V] "

// 调试宏核心函数（简化版本，不依赖时间）
static inline int debug_print(debug_level_t level, const char* tag, const char* format, ...) {
    // 检查调试级别是否启用
    if (level > DEFAULT_DEBUG_LEVEL) {
        return 0;
    }

    // 根据级别选择前缀
    const char* prefix = "";
    switch (level) {
        case DEBUG_LEVEL_ERROR:
            prefix = DEBUG_PREFIX_ERROR;
            break;
        case DEBUG_LEVEL_WARN:
            prefix = DEBUG_PREFIX_WARN;
            break;
        case DEBUG_LEVEL_INFO:
            prefix = DEBUG_PREFIX_INFO;
            break;
        case DEBUG_LEVEL_DEBUG:
            prefix = DEBUG_PREFIX_DEBUG;
            break;
        case DEBUG_LEVEL_VERBOSE:
            prefix = DEBUG_PREFIX_VERBOSE;
            break;
        default:
            return 0;
    }

    // 直接使用printf，避免系统日志缓冲区限制
    printf("%s", prefix);

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

// 修改：大容量日志输出函数（支持分段输出，每次200字符）
static inline int debug_print_long(debug_level_t level, const char* tag, const char* format, ...) {
    // 检查调试级别是否启用
    if (level > DEFAULT_DEBUG_LEVEL) {
        return 0;
    }

    // 根据级别选择前缀
    const char* prefix = "";
    switch (level) {
        case DEBUG_LEVEL_ERROR:
            prefix = DEBUG_PREFIX_ERROR;
            break;
        case DEBUG_LEVEL_WARN:
            prefix = DEBUG_PREFIX_WARN;
            break;
        case DEBUG_LEVEL_INFO:
            prefix = DEBUG_PREFIX_INFO;
            break;
        case DEBUG_LEVEL_DEBUG:
            prefix = DEBUG_PREFIX_DEBUG;
            break;
        case DEBUG_LEVEL_VERBOSE:
            prefix = DEBUG_PREFIX_VERBOSE;
            break;
        default:
            return 0;
    }

    // 处理格式化参数
    va_list args;
    va_start(args, format);
    
    // 使用更大的缓冲区来处理长文本
    char buffer[2048];  // 增加到2KB缓冲区
    int written = vsnprintf(buffer, sizeof(buffer), format, args);
    
    va_end(args);

    if (written < 0) {
        // 格式化失败
        printf("%s", prefix);
        if (tag != NULL && strlen(tag) > 0) {
            printf("[%s] ", tag);
        }
        printf("格式化失败%s\n", COLOR_RESET);
        return 0;
    }

    // 如果文本长度小于等于200字符，直接输出
    if (written <= 200) {
        printf("%s", prefix);
        if (tag != NULL && strlen(tag) > 0) {
            printf("[%s] ", tag);
        }
        printf("%s%s\n", buffer, COLOR_RESET);
        return written;
    }

    // 分段输出长文本（每次200字符）
    const size_t CHUNK_SIZE = 200;
    size_t total_chars = written;
    size_t chunk_count = (total_chars + CHUNK_SIZE - 1) / CHUNK_SIZE; // 向上取整
    
    int total_printed = 0;
    
    for (size_t i = 0; i < chunk_count; i++) {
        size_t chunk_start = i * CHUNK_SIZE;
        size_t chunk_len = (chunk_start + CHUNK_SIZE <= total_chars) ? CHUNK_SIZE : (total_chars - chunk_start);
        
        // 输出前缀和标签（只在第一段输出）
        if (i == 0) {
            printf("%s", prefix);
            if (tag != NULL && strlen(tag) > 0) {
                printf("[%s] ", tag);
            }
        } else {
            // 后续段落的缩进
            printf("    ");
        }
        
        // 分段输出内容
        printf("%.*s", (int)chunk_len, buffer + chunk_start);
        
        // 如果是最后一段，输出颜色重置和换行
        if (i == chunk_count - 1) {
            printf("%s\n", COLOR_RESET);
        } else {
            printf("\n"); // 中间段落换行
        }
        
        total_printed += chunk_len;
    }
    
    return total_printed;
}

// 新增：分段输出长文本的专用宏
#define log_long(level, tag, format, ...) debug_print_long(level, tag, format, ##__VA_ARGS__)
#define log_i_long(tag, format, ...) debug_print_long(DEBUG_LEVEL_INFO, tag, format, ##__VA_ARGS__)
#define log_e_long(tag, format, ...) debug_print_long(DEBUG_LEVEL_ERROR, tag, format, ##__VA_ARGS__)
#define log_w_long(tag, format, ...) debug_print_long(DEBUG_LEVEL_WARN, tag, format, ##__VA_ARGS__)


// 通用调试宏定义
#define log_e(tag, format, ...) debug_print(DEBUG_LEVEL_ERROR, tag, format, ##__VA_ARGS__)
#define log_w(tag, format, ...) debug_print(DEBUG_LEVEL_WARN, tag, format, ##__VA_ARGS__)
#define log_i(tag, format, ...) debug_print(DEBUG_LEVEL_INFO, tag, format, ##__VA_ARGS__)
#define log_d(tag, format, ...) debug_print(DEBUG_LEVEL_DEBUG, tag, format, ##__VA_ARGS__)
#define log_v(tag, format, ...) debug_print(DEBUG_LEVEL_VERBOSE, tag, format, ##__VA_ARGS__)

// 条件调试宏
#define log_if(condition, level, tag, format, ...)          \
    do {                                                    \
        if (condition) {                                    \
            debug_print(level, tag, format, ##__VA_ARGS__); \
        }                                                   \
    } while (0)

// 断言宏
#define debug_assert(condition, format, ...)                                  \
    do {                                                                      \
        if (!(condition)) {                                                   \
            log_e("ASSERT", "Assertion failed at %s:%d", __FILE__, __LINE__); \
            log_e("ASSERT", format, ##__VA_ARGS__);                           \
            while (1) { /* 挂起系统 */                                        \
            }                                                                 \
        }                                                                     \
    } while (0)

#endif  // _DEBUG_H_