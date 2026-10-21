#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Video Converter for STM32H7 MJPEG Player
========================================
将 video/ 目录中的任意视频批量转换为 STM32H7 硬件 MJPEG 解码器可播放的 AVI 格式，
输出到 avi/ 目录。

H743 固定参数:
  - 质量: 2/31 (硬件JPEG解码器最佳质量)
  - 分辨率: 自适应, 最大 1024×600, 宽度 16 字节对齐
  - 音频: PCM 16bit 44100Hz 立体声
  - 编码: MJPEG + YUV 4:2:0

用法:
  python convert_video.py                             # 扫描 video/ → 输出到 avi/
  python convert_video.py --fps 15                    # 指定帧率
  python convert_video.py input.mp4                   # 单文件转换
  python convert_video.py *.mp4                       # 通配符批量
  python convert_video.py -i my_videos -o my_avi      # 指定输入/输出目录
"""

import argparse
import subprocess
import sys
import os
import glob
import json

# 脚本所在目录（用于计算默认 video/ avi/ 路径）
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# ── H743 固定参数 ──
FIXED_QUALITY = 2          # JPEG 质量 (1-31, 越小越好)
MAX_WIDTH = 1024           # 最大宽度
MAX_HEIGHT = 600           # 最大高度

# 支持的视频扩展名
VIDEO_EXTS = {".mp4", ".avi", ".mov", ".mkv", ".webm", ".flv", ".wmv",
              ".m4v", ".mpg", ".mpeg", ".3gp", ".ts", ".mts", ".m2ts",
              ".ogv", ".asf", ".rmvb", ".vob", ".divx"}


def parse_args():
    p = argparse.ArgumentParser(exit_on_error=False,
        description="转换视频为 STM32H7 MJPEG Player 兼容格式 (AVI/MJPEG)",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
H743 固定参数:
  质量: 2/31    分辨率: ≤1024×600, 16对齐    音频: PCM 44100Hz 立体声

示例:
  %(prog)s                                        # 扫描 video/ 目录, 输出到 avi/
  %(prog)s --fps 15                                # 指定帧率
  %(prog)s input.mp4 -o output.avi                # 单文件转换
  %(prog)s -i ./videos -o ./output                # 指定目录
        """)
    p.add_argument("inputs", nargs="*",
                   help="输入视频文件 (可选, 默认扫描 video/ 目录)")
    p.add_argument("-i", "--input-dir", default=None,
                   help="输入目录 (默认: 脚本所在目录下的 video/)")
    p.add_argument("-o", "--output", default=None,
                   help="输出目录或文件 (默认: 脚本所在目录下的 avi/)")
    p.add_argument("--fps", type=float, default=0,
                   help="输出帧率 (0=保持原始)")
    p.add_argument("--start", type=str, default=None,
                   help="起始时间 (如 00:01:30)")
    p.add_argument("--duration", type=str, default=None,
                   help="持续时间 (如 00:00:10)")
    p.add_argument("--overwrite", action="store_true",
                   help="覆盖已存在的输出文件 (默认跳过)")
    return p.parse_args()


def check_ffmpeg():
    try:
        subprocess.run(["ffmpeg", "-version"], capture_output=True, check=True)
    except (FileNotFoundError, subprocess.CalledProcessError):
        print("[ERROR] ffmpeg 未安装或不在 PATH 中")
        print("  下载: https://ffmpeg.org/download.html")
        print("  或: winget install ffmpeg")
        sys.exit(1)


def align16(w):
    """宽度 16 字节对齐 (MJPEG 解码器要求)"""
    return (w + 15) // 16 * 16


def is_video_file(path):
    """判断文件是否为支持的视频格式"""
    ext = os.path.splitext(path)[1].lower()
    return ext in VIDEO_EXTS


def scan_video_dir(input_dir):
    """扫描目录中的所有视频文件"""
    if not os.path.isdir(input_dir):
        return []
    files = []
    for f in os.listdir(input_dir):
        full = os.path.join(input_dir, f)
        if os.path.isfile(full) and is_video_file(f):
            files.append(full)
    return sorted(files)


def convert_one(input_path, output_path, fps, start, duration, overwrite):
    name = os.path.basename(input_path)
    print(f"\n{'='*60}")
    print(f"  输入: {name}")

    # Check if output exists
    if os.path.exists(output_path) and not overwrite:
        size_mb = os.path.getsize(output_path) / (1024 * 1024)
        print(f"  [SKIP] 已存在: {os.path.basename(output_path)} ({size_mb:.1f} MB)")
        return None  # skipped

    # Probe input
    probe_cmd = [
        "ffprobe", "-v", "quiet", "-print_format", "json",
        "-show_format", "-show_streams", input_path
    ]
    try:
        info = subprocess.run(probe_cmd, capture_output=True, text=True, timeout=15)
    except subprocess.TimeoutExpired:
        print("[WARN] 无法探测文件信息, 使用默认 480×272")
        info = None

    # ── 自动检测分辨率, 缩放到 H743 范围内 ──
    width = 0
    height = 0
    if info and info.returncode == 0:
        try:
            data = json.loads(info.stdout)
            for s in data.get("streams", []):
                if s.get("codec_type") == "video":
                    width = s.get("width", 0)
                    height = s.get("height", 0)
                    break
        except json.JSONDecodeError:
            pass

    if width <= 0 or height <= 0:
        width, height = 480, 272  # fallback

    # 等比缩放到 1024×600 以内
    h_scale = height / MAX_HEIGHT if height > MAX_HEIGHT else 1.0
    w_scale = width / MAX_WIDTH if width > MAX_WIDTH else 1.0
    scale = max(h_scale, w_scale)
    if scale > 1.0:
        width = int(width / scale)
        height = int(height / scale)

    # 宽度 16 字节对齐, 高度偶数
    orig_w = width
    width = align16(width)
    if width != orig_w:
        print(f"  宽度对齐: {orig_w} → {width} (16字节对齐)")
    if height % 2 != 0:
        height = height - (height % 2)

    print(f"  分辨率: {width}×{height}")
    print(f"  质量:   {FIXED_QUALITY}/31 (H743 固定)")
    print(f"  输出:   {os.path.basename(output_path)}")

    # ── 构建 ffmpeg 命令 ──
    cmd = ["ffmpeg", "-y", "-i", input_path]

    if start:
        cmd += ["-ss", start]
    if duration:
        cmd += ["-t", duration]

    cmd += ["-vf", f"scale={width}:{height}"]

    if fps > 0:
        cmd += ["-r", str(fps)]

    cmd += [
        "-vcodec", "mjpeg",
        "-q:v", str(FIXED_QUALITY),
        "-pix_fmt", "yuvj420p",
        # 音频: PCM 16bit 44100Hz 立体声
        "-acodec", "pcm_s16le",
        "-ar", "44100",
        "-ac", "2",
        "-f", "avi",
    ]
    cmd.append(output_path)

    try:
        result = subprocess.run(
            cmd, capture_output=True, text=True, timeout=600
        )
        if result.returncode == 0:
            size_mb = os.path.getsize(output_path) / (1024 * 1024)
            print(f"  [OK]   {os.path.basename(output_path)}  ({size_mb:.1f} MB)")
            return True
        else:
            stderr_short = result.stderr.strip().split("\n")[-3:]
            print(f"  [FAIL] ffmpeg 错误:")
            for line in stderr_short:
                print(f"         {line}")
            return False
    except subprocess.TimeoutExpired:
        print("  [FAIL] 转换超时")
        return False
    except Exception as e:
        print(f"  [FAIL] {e}")
        return False


def main():
    try:
        args = parse_args()
    except argparse.ArgumentError as e:
        print(f"\n[ERROR] {e}", file=sys.stderr)
        print("用法:   python convert_video.py                         # 扫描 video/ → avi/", file=sys.stderr)
        print("       python convert_video.py input.mp4                # 单文件", file=sys.stderr)
        print("       python convert_video.py --help                   # 查看帮助", file=sys.stderr)
        sys.exit(2)

    check_ffmpeg()

    # ── 确定输入文件列表 ──
    input_dir = args.input_dir or os.path.join(SCRIPT_DIR, "video")
    output_target = args.output or os.path.join(SCRIPT_DIR, "avi")

    if args.inputs:
        inputs = []
        for pat in args.inputs:
            matched = glob.glob(pat)
            if matched:
                inputs.extend(matched)
            else:
                inputs.append(pat)
    else:
        inputs = scan_video_dir(input_dir)
        if not inputs:
            print(f"[ERROR] 输入目录 '{input_dir}' 中未找到视频文件")
            print(f"  支持的格式: {', '.join(sorted(VIDEO_EXTS))}")
            sys.exit(1)

    if not inputs:
        print("[ERROR] 未找到输入文件")
        sys.exit(1)

    # ── 确定输出策略 ──
    single_file = len(inputs) == 1

    if single_file and args.output and not os.path.isdir(args.output):
        out_dir = os.path.dirname(os.path.abspath(args.output)) or "."
        out_pairs = [(inputs[0], os.path.abspath(args.output))]
    else:
        out_dir = os.path.abspath(output_target)
        out_pairs = []
        for inp in inputs:
            base = os.path.splitext(os.path.basename(inp))[0]
            out_pairs.append((inp, os.path.join(out_dir, base + ".avi")))

    os.makedirs(out_dir, exist_ok=True)

    # ── 批量转换 ──
    print(f"\n{'='*60}")
    print(f"  STM32H7 视频转换器")
    print(f"  输入: {len(inputs)} 个文件")
    print(f"  输出: {out_dir}")
    print(f"  质量: {FIXED_QUALITY}/31 (固定)")
    print(f"  音频: PCM 44100Hz 立体声 (固定)")
    if args.fps > 0:
        print(f"  帧率: {args.fps}")
    print(f"{'='*60}")

    success = 0
    skipped = 0
    total = len(inputs)

    for inp, out in out_pairs:
        if not os.path.exists(inp):
            print(f"\n[SKIP] 文件不存在: {inp}")
            skipped += 1
            continue

        result = convert_one(inp, out, args.fps, args.start, args.duration,
                            args.overwrite)
        if result is True:
            success += 1
        elif result is None:
            skipped += 1

    # ── 汇总 ──
    print(f"\n{'='*60}")
    parts = [f"成功: {success}"]
    if skipped > 0:
        parts.append(f"跳过: {skipped}")
    failed = total - success - skipped
    if failed > 0:
        parts.append(f"失败: {failed}")
    print(f"  完成  ({', '.join(parts)})  —  输出目录: {out_dir}")
    print(f"{'='*60}")

    if failed > 0:
        sys.exit(1)


if __name__ == "__main__":
    try:
        main()
    except SystemExit as e:
        sys.exit(e.code)
