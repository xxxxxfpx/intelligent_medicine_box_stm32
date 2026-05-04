"""
汉字点阵生成器 - 测试脚本
将汉字转换为 OLED 可用的点阵数组，并可视化显示
"""

from PIL import Image, ImageDraw, ImageFont
import numpy as np


def char_to_bitmap(char, font_path=None, size=16):
    """
    将单个汉字转换为点阵
    
    Args:
        char: 要转换的汉字，如 "温"
        font_path: 字体文件路径，None 使用默认字体
        size: 点阵大小（16x16）
    
    Returns:
        点阵数据数组（32字节）
    """
    # 创建空白图像（黑色背景）
    image = Image.new('1', (size, size), 0)
    draw = ImageDraw.Draw(image)
    
    # 尝试加载字体
    font = None
    if font_path:
        try:
            font = ImageFont.truetype(font_path, size - 2)  # 稍微小一点，避免溢出
        except Exception as e:
            print(f"  警告: 无法加载字体 {font_path}: {e}")
    
    # 如果指定字体失败，尝试系统默认中文字体
    if font is None:
        # 尝试常见的中文字体路径
        system_fonts = [
            "C:/Windows/Fonts/simhei.ttf",      # Windows 黑体
            "C:/Windows/Fonts/simsun.ttc",      # Windows 宋体
            "C:/Windows/Fonts/msyh.ttc",        # Windows 微软雅黑
            "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",  # Linux
            "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
            "/System/Library/Fonts/PingFang.ttc",  # macOS
        ]
        for sys_font in system_fonts:
            try:
                font = ImageFont.truetype(sys_font, size - 2)
                print(f"  使用系统字体: {sys_font}")
                break
            except:
                continue
    
    # 如果都失败了，使用默认字体
    if font is None:
        font = ImageFont.load_default()
        print("  警告: 使用默认字体，可能无法显示中文")
    
    # 计算文字居中位置
    bbox = draw.textbbox((0, 0), char, font=font)
    text_width = bbox[2] - bbox[0]
    text_height = bbox[3] - bbox[1]
    x = (size - text_width) // 2
    y = (size - text_height) // 2 - bbox[1]
    
    # 绘制文字（白色）
    draw.text((x, y), char, fill=1, font=font)
    
    # 转换为 numpy 数组
    pixels = np.array(image, dtype=np.uint8)
    
    # 生成横向取模数据（每行2字节，16行，共32字节）
    bitmap = []
    for row in pixels:
        byte1 = 0
        byte2 = 0
        # 前8列 -> byte1
        for i in range(8):
            if row[i]:
                byte1 |= (1 << (7 - i))
        # 后8列 -> byte2
        for i in range(8, 16):
            if row[i]:
                byte2 |= (1 << (15 - i))
        bitmap.append(byte1)
        bitmap.append(byte2)
    
    return bitmap, pixels


def print_bitmap_visual(bitmap, size=16):
    """
    在控制台可视化打印点阵
    
    Args:
        bitmap: 32字节的点阵数据
        size: 点阵大小
    """
    print("  点阵预览 (█=亮,  =灭):")
    print("  +" + "-" * size + "+")
    
    for row in range(size):
        line = "  |"
        byte1 = bitmap[row * 2]
        byte2 = bitmap[row * 2 + 1]
        
        # 前8位
        for bit in range(8):
            if byte1 & (1 << (7 - bit)):
                line += "█"
            else:
                line += " "
        
        # 后8位
        for bit in range(8):
            if byte2 & (1 << (7 - bit)):
                line += "█"
            else:
                line += " "
        
        line += "|"
        print(line)
    
    print("  +" + "-" * size + "+")


def format_c_array(name, bitmap):
    """
    格式化为 C 语言数组
    
    Args:
        name: 数组名称
        bitmap: 32字节的点阵数据
    
    Returns:
        C 语言格式的字符串
    """
    lines = [f"const uint8_t {name}[] = {{"]
    hex_values = []
    for i, byte in enumerate(bitmap):
        hex_values.append(f"0x{byte:02x}")
    
    # 每行8个值
    for i in range(0, len(hex_values), 8):
        row = ", ".join(hex_values[i:i+8])
        lines.append(f"    {row},")
    
    lines.append("};")
    return "\n".join(lines)


def test_single_char(char, font_path=None):
    """
    测试单个汉字
    """
    print(f"\n{'='*50}")
    print(f"生成汉字: [{char}]")
    print(f"{'='*50}")
    
    # 生成点阵
    bitmap, pixels = char_to_bitmap(char, font_path)
    
    # 显示可视化
    print_bitmap_visual(bitmap)
    
    # 显示原始数据
    print(f"\n  原始数据 (32字节):")
    for i in range(0, 32, 8):
        hex_str = " ".join([f"0x{b:02x}" for b in bitmap[i:i+8]])
        print(f"    {hex_str}")
    
    # 生成 C 数组
    pinyin_map = {
        '温': 'Wen', '度': 'Du', '时': 'Shi', '间': 'Jian',
        '定': 'Ding', '位': 'Wei', '体': 'Ti', '感': 'Gan',
        '温': 'Wen', '湿': 'Shi', '度': 'Du', '年': 'Nian',
        '月': 'Yue', '日': 'Ri', '时': 'Shi', '分': 'Fen',
        '秒': 'Miao', '定': 'Ding', '位': 'Wei', '置': 'Zhi',
        '状': 'Zhuang', '态': 'Tai', '网': 'Wang', '络': 'Luo',
    }
    name = pinyin_map.get(char, f"Char_{ord(char):04X}")
    
    print(f"\n  C 语言数组:")
    c_code = format_c_array(f"Chinese_{name}", bitmap)
    print(c_code)
    
    return bitmap, c_code


def generate_font_library(chars, output_file=None):
    """
    批量生成字库
    """
    print(f"\n{'='*60}")
    print(f"批量生成字库: {chars}")
    print(f"{'='*60}")
    
    results = []
    for char in chars:
        bitmap, c_code = test_single_char(char)
        results.append((char, c_code))
    
    # 保存到文件
    if output_file:
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write("/**\n")
            f.write(" * 中文点阵字库 - 16x16\n")
            f.write(" * 自动生成，横向取模\n")
            f.write(" */\n\n")
            f.write("#ifndef __CHINESE_FONT_H\n")
            f.write("#define __CHINESE_FONT_H\n\n")
            f.write("#include <stdint.h>\n\n")
            
            for char, c_code in results:
                f.write(f"// 汉字: {char}\n")
                f.write(c_code)
                f.write("\n\n")
            
            f.write("#endif /* __CHINESE_FONT_H */\n")
        
        print(f"\n{'='*60}")
        print(f"字库已保存到: {output_file}")
        print(f"{'='*60}")


# ============ 主程序 ============
if __name__ == "__main__":
    print("="*60)
    print("  汉字点阵生成器 - 测试程序")
    print("="*60)
    print("\n本程序将汉字转换为 OLED 16x16 点阵数组")
    print("支持可视化预览和 C 语言代码生成")
    
    # 测试单个汉字
    test_chars = [
        "温",  # 温度
        "度",
        "时",  # 时间
        "间",
        "定",  # 定位
        "位",
    ]
    
    print("\n\n准备测试以下汉字:")
    for i, c in enumerate(test_chars, 1):
        print(f"  {i}. {c}")
    
    # 执行测试
    for char in test_chars:
        test_single_char(char)
    
    # 批量生成字库文件
    generate_font_library("温度时间定位", "chinese_font.h")
    
    print("\n\n" + "="*60)
    print("  测试完成!")
    print("="*60)
    print("\n生成的字库文件: chinese_font.h")
    print("可将该文件复制到 STM32 项目中使用")
