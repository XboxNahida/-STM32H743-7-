## 此脚本用于将C文件的图片，字体转化为bin的二进制文件

> 使用步骤
1. 在脚本所在位置新建两个文件夹：c_files 和 bin_files。

2. 将你所有的图片数组、字体数组的 .c 文件放入 c_files 文件夹中。

3. 用Visual Studio Code打开脚本，按F5运行脚本：python c_to_bin.py。

4. 运行结束后，打开 bin_files 文件夹，你就能看到对应的纯二进制 .bin 文件了。将这些文件直接复制到你的 SD 卡中，后续将其转移到W25Q外部Flash。

## 脚本二convert_font_to_bin.ps1

> lvgl官方转化工具，将ttf转化为bin