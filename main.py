#!/usr/bin/env python3
# -*- coding: utf-8 -*-

def convert_font_to_c_array(input_file, output_file):
    """
    将取模数据.txt文件转换为C语言数组格式
    """
    try:
        with open(input_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 分割文件内容为行
        lines = content.split('\n')
        
        # 准备输出数组
        c_array_lines = []
        char_count = 0
        
        i = 0
        while i < len(lines):
            line = lines[i].strip()
            
            # 查找字符注释行
            if line.startswith('/*--  文字:'):
                # 提取字符信息
                char_info = line
                
                # 查找对应的点阵数据行（跳过说明行）
                j = i + 1
                while j < len(lines) and not lines[j].strip().startswith('0x'):
                    j += 1
                
                if j < len(lines):
                    # 找到点阵数据行
                    data_line = lines[j].strip()
                    
                    # 添加到C数组
                    c_array_lines.append(char_info)
                    c_array_lines.append(data_line + ',')
                    char_count += 1
                    
                    i = j + 1
                else:
                    i += 1
            else:
                i += 1
        
        # 生成C语言数组头
        c_header = f"""// 宋体10号字体点阵数据（8x13）
// 从 {input_file} 转换而来
// 总共 {char_count} 个字符

const uint8_t songti_font_10x13[][13] = {{
"""
        
        # 生成C语言数组尾
        c_footer = "};"
        
        # 写入输出文件
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(c_header)
            for line in c_array_lines:
                f.write(line + '\n')
            f.write(c_footer)
        
        print(f"转换完成！共处理 {char_count} 个字符")
        print(f"输出文件: {output_file}")
        
    except Exception as e:
        print(f"转换过程中出现错误: {e}")

def main():
    # 输入文件路径
    input_file = r"f:\DevEcoProjects\hi3861_proj\src\applications\sample\wifi-iot\app\startup\src\fonts\取模数据.txt"
    
    # 输出文件路径
    output_file = r"f:\DevEcoProjects\hi3861_proj\src\applications\sample\wifi-iot\app\startup\src\fonts\songti_font_array.c"
    
    # 执行转换
    convert_font_to_c_array(input_file, output_file)

if __name__ == "__main__":
    main()