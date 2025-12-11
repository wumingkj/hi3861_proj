#ifndef __SONGTI_FONT_H__
#define __SONGTI_FONT_H__

#include <stdint.h>

// 宋体10号字体配置
#define SONGTI_FONT_WIDTH    8           // 字体宽度（调整为8的倍数）
#define SONGTI_FONT_HEIGHT   13          // 字体高度
#define SONGTI_FONT_BYTES_PER_CHAR 13    // 每个字符占用的字节数

// 字体字符范围
#define SONGTI_FONT_START_CHAR 0x20      // 起始字符（空格）
#define SONGTI_FONT_END_CHAR   0x7E      // 结束字符（~）

// 宋体10号字体点阵数据（基于您提供的格式）
extern const uint8_t songti_font_10x13[][SONGTI_FONT_BYTES_PER_CHAR];

// 字体操作函数
uint8_t songti_font_get_char_width(char c);
uint8_t songti_font_get_char_height(void);
const uint8_t* songti_font_get_char_data(char c);

#endif /* __SONGTI_FONT_H__ */