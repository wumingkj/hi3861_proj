# 字体库模块

## 模块概述

字体库模块提供中文字体支持，目前包含宋体10号字体，用于OLED显示模块的文本渲染。该模块支持ASCII字符集和部分中文字符的显示。

## 功能特性

- ✅ **宋体10号字体** - 8x13像素点阵字体
- ✅ **ASCII字符支持** - 支持空格到波浪线（0x20-0x7E）
- ✅ **中文字符支持** - 支持常用中文字符
- ✅ **内存优化** - 紧凑的字体数据存储
- ✅ **快速渲染** - 优化的字符查找和渲染算法

## 字体规格

### 宋体10号字体参数
```c
#define SONGTI_FONT_WIDTH    8           // 字体宽度（8的倍数）
#define SONGTI_FONT_HEIGHT   13          // 字体高度
#define SONGTI_FONT_BYTES_PER_CHAR 13    // 每个字符占用的字节数
```

### 字符范围
- **ASCII字符**: 0x20（空格）到 0x7E（~）
- **中文字符**: 常用汉字字符集

## API接口

### 字体信息获取
```c
/**
 * @brief 获取字符宽度
 * @param c 要查询的字符
 * @return 字符宽度（像素）
 */
uint8_t songti_font_get_char_width(char c);

/**
 * @brief 获取字体高度
 * @return 字体高度（像素）
 */
uint8_t songti_font_get_char_height(void);

/**
 * @brief 获取字符点阵数据
 * @param c 要获取的字符
 * @return 字符点阵数据指针，NULL表示字符不存在
 */
const uint8_t* songti_font_get_char_data(char c);
```

## 使用示例

### 基本使用
```c
#include "songti_font.h"

// 获取字体信息
uint8_t width = songti_font_get_char_width('A');
uint8_t height = songti_font_get_char_height();
printf("字体尺寸: %dx%d\n", width, height);

// 获取字符数据
const uint8_t* char_data = songti_font_get_char_data('H');
if (char_data != NULL) {
    // 使用字符数据进行渲染
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (char_data[y] & (1 << (7 - x))) {
                // 绘制像素点
            }
        }
    }
}
```

### 与OLED模块集成
```c
// 在OLED显示文本
void oled_draw_text(uint8_t x, uint8_t y, const char* text) {
    uint8_t cursor_x = x;
    
    while (*text) {
        const uint8_t* char_data = songti_font_get_char_data(*text);
        if (char_data != NULL) {
            // 渲染字符到OLED
            oled_draw_char(cursor_x, y, char_data);
            cursor_x += songti_font_get_char_width(*text) + 1; // 字符间距
        }
        text++;
    }
}
```

## 内部实现

### 字体数据存储
字体数据以二维数组形式存储：
```c
// 每个字符13字节，每字节表示一行的像素数据
const uint8_t songti_font_10x13[][13] = {
    // 空格字符 (0x20)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // 感叹号字符 (0x21)
    {0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18},
    // ... 其他字符
};
```

### 字符查找算法
通过字符ASCII码直接索引字体数组：
```c
const uint8_t* songti_font_get_char_data(char c) {
    if (c < SONGTI_FONT_START_CHAR || c > SONGTI_FONT_END_CHAR) {
        return NULL; // 字符超出范围
    }
    return songti_font_10x13[c - SONGTI_FONT_START_CHAR];
}
```

## 性能优化

### 内存效率
- **紧凑存储**: 每个字符仅占用13字节
- **直接索引**: O(1)时间复杂度的字符查找
- **无动态分配**: 所有数据静态存储

### 渲染优化
- **位操作**: 使用位运算快速判断像素状态
- **缓存友好**: 连续内存访问模式
- **最小化计算**: 预计算的字符尺寸信息

## 扩展性

### 添加新字体
模块设计支持多种字体扩展：
```c
// 可以添加其他字体定义
extern const uint8_t heiti_font_12x16[][16];
extern const uint8_t kaiti_font_10x14[][14];
```

### 多语言支持
当前支持ASCII字符集，可扩展支持：
- 更多中文字符
- 其他语言字符
- 特殊符号

## 注意事项

1. **字符范围**: 仅支持定义范围内的字符
2. **内存占用**: 字体数据占用固定Flash空间
3. **渲染兼容**: 需要与显示模块的像素格式匹配
4. **性能考虑**: 大量文本渲染时注意性能优化

## 相关文件

- `songti_font.h` - 字体头文件，包含API声明和配置
- `songti_font.c` - 字体实现文件，包含完整的字体数据

## 未来计划

- 🔄 **更多字体**: 添加黑体、楷体等常用字体
- 🔄 **字符扩展**: 支持更多中文字符和特殊符号
- 🔄 **动态字体**: 支持运行时字体加载
- 🔄 **字体缩放**: 支持字体尺寸缩放功能

[返回主文档](../../../../../../../../README.md)