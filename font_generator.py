"""
汉字点阵生成器 - 列扫描格式（匹配OLED Page Addressing Mode + 0xC8 COM remap）
将汉字转换为 OLED 可用的点阵数组，并可视化显示

OLED 数据格式说明:
- SSD1306 Page Addressing Mode，每页8行
- 初始化使用 0xC8 (COM remap), 所以:
  - bit0 = 该页顶部行
  - bit7 = 该页底部行
- 前16字节 = 页0 (行0-7), 每列1字节
- 后16字节 = 页1 (行8-15), 每列1字节
"""

from PIL import Image, ImageDraw, ImageFont
import numpy as np


def char_to_bitmap(char, font_path=None, size=16):
    """
    将单个汉字转换为OLED列扫描格式的点阵
    
    OLED Page Addressing Mode:
    - 每个字节 = 1列的8垂直像素
    - 字节0-15 = 列0-15的上半部（行0-7）
    - 字节16-31 = 列0-15的下半部（行8-15）
    - bit7 = 该页最上方像素
    
    Args:
        char: 要转换的汉字
        font_path: 字体文件路径
        size: 点阵大小（16x16）
    
    Returns:
        (bitmap_32bytes, pixels_16x16_array)
    """
    image = Image.new('1', (size, size), 0)
    draw = ImageDraw.Draw(image)
    
    font = None
    if font_path:
        try:
            font = ImageFont.truetype(font_path, size - 2)
        except Exception as e:
            print(f"  警告: 无法加载字体 {font_path}: {e}")
    
    if font is None:
        system_fonts = [
            "C:/Windows/Fonts/simhei.ttf",
            "C:/Windows/Fonts/simsun.ttc",
            "C:/Windows/Fonts/msyh.ttc",
            "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",
            "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
            "/System/Library/Fonts/PingFang.ttc",
        ]
        for sys_font in system_fonts:
            try:
                font = ImageFont.truetype(sys_font, size - 2)
                print(f"  使用系统字体: {sys_font}")
                break
            except:
                continue
    
    if font is None:
        font = ImageFont.load_default()
        print("  警告: 使用默认字体，可能无法显示中文")
    
    bbox = draw.textbbox((0, 0), char, font=font)
    text_width = bbox[2] - bbox[0]
    text_height = bbox[3] - bbox[1]
    x = (size - text_width) // 2
    y = (size - text_height) // 2 - bbox[1]
    
    draw.text((x, y), char, fill=1, font=font)
    
    pixels = np.array(image, dtype=np.uint8)  # pixels[row][col]
    
    bitmap = []
    
    # 上半部（行0-7）：列0-15
    # bit0=行0(顶)，bit7=行7(底) — 匹配OLED 0xC8 COM remap
    for col in range(16):
        byte_val = 0
        for row in range(8):
            if pixels[row][col]:
                byte_val |= (1 << row)  # bit0=行0(顶)
        bitmap.append(byte_val)
    
    # 下半部（行8-15）：列0-15
    # bit0=行8(顶)，bit7=行15(底)
    for col in range(16):
        byte_val = 0
        for row in range(8, 16):
            if pixels[row][col]:
                byte_val |= (1 << (row - 8))  # bit0=行8(顶)
        bitmap.append(byte_val)
    
    return bitmap, pixels


def print_bitmap_visual(bitmap, size=16):
    """在控制台可视化打印点阵"""
    print("  点阵预览 (█=亮,  =灭):")
    print("  注意: bit0=行顶(顶), bit7=行底(底)")
    print("  +" + "-" * size + "+")
    
    for row in range(size):
        line = "  |"
        for col in range(size):
            byte_idx = (col if row < 8 else col + 16)
            shift = row if row < 8 else (row - 8)  # bit0=顶
            if bitmap[byte_idx] & (1 << shift):
                line += "█"
            else:
                line += " "
        line += "|"
        print(line)
    
    print("  +" + "-" * size + "+")


def format_c_array(name, bitmap):
    """格式化为 C 语言数组"""
    lines = [f"const uint8_t {name}[] = {{"]
    hex_values = [f"0x{b:02x}" for b in bitmap]
    for i in range(0, len(hex_values), 8):
        row = ", ".join(hex_values[i:i+8])
        lines.append(f"    {row},")
    lines.append("};")
    return "\n".join(lines)


def test_single_char(char, font_path=None):
    """测试单个汉字"""
    print(f"\n{'='*50}")
    print(f"生成汉字: [{char}]")
    print(f"{'='*50}")
    
    bitmap, pixels = char_to_bitmap(char, font_path)
    
    print_bitmap_visual(bitmap)
    
    pinyin_map = {
        '温': 'Wen', '度': 'Du', '时': 'Shi', '间': 'Jian',
        '定': 'Ding', '位': 'Wei', '年': 'Nian', '月': 'Yue',
        '日': 'Ri', '物': 'Wu', '环': 'Huan', '境': 'Jing',
        '状': 'Zhuang', '态': 'Tai',
        '连': 'Lian', '接': 'Jie', '中': 'Zhong',
        '离': 'Li', '线': 'Xian', '在': 'Zai',
        '有': 'You', '效': 'Xiao', '搜': 'Sou',
        '索': 'Suo', '失': 'Shi2', '败': 'Bai',
        '网': 'Wang', '络': 'Luo2',
        '经': 'Jing2', '纬': 'Wei2',
    }
    name = pinyin_map.get(char, f"Char_{ord(char):04X}")

    c_code = format_c_array(f"Chinese_{name}", bitmap)
    print(f"\n  C 语言数组 (列扫描格式):")
    print(c_code)
    
    return bitmap, c_code


def generate_font_c_file(chars, output_file=None):
    """批量生成字库 .c 文件"""
    results = []
    for char in chars:
        bitmap, _ = test_single_char(char)
        results.append((char, bitmap))
    
    if output_file:
        pinyin_map = {
            '温': 'Wen', '度': 'Du', '时': 'Shi', '间': 'Jian',
            '定': 'Ding', '位': 'Wei', '年': 'Nian', '月': 'Yue',
            '日': 'Ri', '物': 'Wu', '环': 'Huan', '境': 'Jing',
            '状': 'Zhuang', '态': 'Tai',
            '连': 'Lian', '接': 'Jie', '中': 'Zhong',
            '离': 'Li', '线': 'Xian', '在': 'Zai',
            '有': 'You', '效': 'Xiao', '搜': 'Sou',
            '索': 'Suo', '失': 'Shi2', '败': 'Bai',
            '网': 'Wang', '络': 'Luo2',
            '经': 'Jing2', '纬': 'Wei2',
        }
        
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write('/**\n')
            f.write(' * 中文点阵字库 - 16x16 列扫描格式\n')
            f.write(' * 匹配 OLED Page Addressing Mode\n')
            f.write(' */\n\n')
            f.write('#include "chinese_font.h"\n\n')
            
            for char, bitmap in results:
                name = pinyin_map.get(char, f"Char_{ord(char):04X}")
                f.write(f'// 汉字: {char}\n')
                f.write(format_c_array(f"Chinese_{name}", bitmap))
                f.write('\n\n')
        
        print(f"\n{'='*60}")
        print(f"字库已保存到: {output_file}")
        print(f"{'='*60}")


# ============ 主程序 ============
if __name__ == "__main__":
    print("="*60)
    print("  汉字点阵生成器 - 列扫描格式")
    print("="*60)
    print("\n生成列扫描格式，匹配 OLED Page Addressing Mode")
    print("每字节 = 1列8垂直像素，bit7=顶")
    
    test_chars = [
        "温", "度", "时", "间", "定", "位",
        "年", "月", "日", "物", "环", "境",
        "状", "态",
        "连", "接", "中", "离", "线", "在",
        "有", "效", "搜", "索", "失", "败",
        "网", "络",
        "经", "纬",
    ]
    
    print(f"\n准备测试以下汉字: {''.join(test_chars)}")
    
    for char in test_chars:
        test_single_char(char)
    
    # 生成 chinese_font.c
    generate_font_c_file(test_chars, "chinese_font.c")
    
    print("\n\n" + "="*60)
    print("  测试完成!")
    print("="*60)
    print("\n生成的文件: chinese_font.c (列扫描格式)")
    print("将该文件复制到 STM32 项目 Hardware 目录")
