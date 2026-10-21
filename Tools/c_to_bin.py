import re
import os


HEX_PATTERN = re.compile(r'0[xX][0-9a-fA-F]{1,2}')
IMAGE_ARRAY_PATTERN = re.compile(
    r'\b(?:uint8_t|unsigned\s+char)\s+[A-Za-z_]\w*_map\s*\[\s*\]\s*=\s*'
    r'\{(?P<body>.*?)\};',
    re.DOTALL,
)


def extract_image_array(content):
    """只提取LVGL生成的 *_map[] 图片数组。"""
    match = IMAGE_ARRAY_PATTERN.search(content)
    if match is None:
        raise ValueError("未找到LVGL图片数组 *_map[]")

    hex_strings = HEX_PATTERN.findall(match.group("body"))
    if not hex_strings:
        raise ValueError("图片数组中没有十六进制数据")
    return bytes(int(hex_str, 16) for hex_str in hex_strings)


def convert_c_to_bin(input_dir, output_dir):
    """
    扫描 input_dir 里的 .c 文件，将LVGL图片数组保存为 .bin 到 output_dir
    """
    # 如果输出目录不存在，则创建
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    # 遍历输入目录
    for filename in os.listdir(input_dir):
        if filename.endswith(".c"):
            c_filepath = os.path.join(input_dir, filename)
            bin_filename = filename.replace(".c", ".bin")
            bin_filepath = os.path.join(output_dir, bin_filename)

            print(f"正在处理: {filename} ...")
            
            try:
                # 读取 C 文件内容 (处理可能的编码问题，尝试 utf-8，不行则用 gbk)
                try:
                    with open(c_filepath, 'r', encoding='utf-8') as f:
                        content = f.read()
                except UnicodeDecodeError:
                    with open(c_filepath, 'r', encoding='gbk') as f:
                        content = f.read()

                try:
                    byte_data = extract_image_array(content)
                except ValueError:
                    print(f"  [跳过] {filename} 不是LVGL图片数组。\n")
                    continue

                # 写入二进制文件
                with open(bin_filepath, 'wb') as f:
                    f.write(byte_data)

                print(f"  [成功] 转换完成 -> {bin_filename} (大小: {len(byte_data)} 字节)\n")

            except Exception as e:
                print(f"  [错误] 处理 {filename} 时发生异常: {e}\n")

if __name__ == "__main__":
    # ---------------- 配置区 ----------------
    # 将你的 .c 文件放在这个文件夹下 (可以使用相对路径或绝对路径)
    INPUT_DIRECTORY = "./c_files" 
    
    # 生成的 .bin 文件会存放在这里
    OUTPUT_DIRECTORY = "./bin_files"
    # ----------------------------------------

    # 确保输入目录存在
    if not os.path.exists(INPUT_DIRECTORY):
        print(f"错误: 找不到输入目录 '{INPUT_DIRECTORY}'。请先创建该目录并放入 .c 文件。")
    else:
        print("开始批量转换...\n" + "-"*30)
        convert_c_to_bin(INPUT_DIRECTORY, OUTPUT_DIRECTORY)
        print("-"*30 + "\n全部转换流程结束！")
