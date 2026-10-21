import importlib.util
import re
import tempfile
import unittest
import zlib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCRIPT_PATH = ROOT / "Tools" / "c_to_bin.py"


def load_converter():
    spec = importlib.util.spec_from_file_location("c_to_bin", SCRIPT_PATH)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def extract_expected_array(source):
    match = re.search(r"_map\s*\[\s*\]\s*=\s*\{(?P<body>.*?)\};", source, re.DOTALL)
    if match is None:
        raise AssertionError("未找到LVGL图片数组")
    values = re.findall(r"0[xX][0-9a-fA-F]{1,2}", match.group("body"))
    return bytes(int(value, 16) for value in values)


class QspiAssetGenerationTests(unittest.TestCase):
    def test_converter_ignores_hex_literals_outside_image_array(self):
        converter = load_converter()
        source = """
/* 0xAA must not become image data. */
const unsigned char sample_map[] = { 0x11, 0x22, 0x33 };
/* 0xBB must not become image data. */
"""

        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = Path(temp_dir)
            input_dir = temp_path / "input"
            output_dir = temp_path / "output"
            input_dir.mkdir()
            (input_dir / "sample.c").write_text(source, encoding="utf-8")

            converter.convert_c_to_bin(str(input_dir), str(output_dir))

            self.assertEqual(
                (output_dir / "sample.bin").read_bytes(),
                bytes((0x11, 0x22, 0x33)),
            )

    def test_checked_in_bins_match_only_the_generated_image_arrays(self):
        names = (
            "_PPC_lock_RGB565_1024x600",
            "_1_RGB565_285x107",
            "_2_RGB565_700x370",
            "_3_RGB565_600x220",
        )

        for name in names:
            with self.subTest(name=name):
                source = (ROOT / "Tools" / "c_files" / f"{name}.c").read_text(
                    encoding="utf-8"
                )
                expected = extract_expected_array(source)
                actual = (ROOT / "Tools" / "bin_files" / f"{name}.bin").read_bytes()
                self.assertEqual(actual, expected)

    def test_firmware_crc_constants_match_checked_in_assets(self):
        source = (ROOT / "Core" / "Peripheral" / "Src" / "norflash.c").read_text(
            encoding="utf-8"
        )
        assets = {
            "QSPI_WALLPAPER_CRC32": "_PPC_lock_RGB565_1024x600.bin",
            "QSPI_IMG1_CRC32": "_1_RGB565_285x107.bin",
            "QSPI_IMG2_CRC32": "_2_RGB565_700x370.bin",
            "QSPI_IMG3_CRC32": "_3_RGB565_600x220.bin",
        }

        for constant, filename in assets.items():
            with self.subTest(constant=constant):
                match = re.search(
                    rf"#define\s+{constant}\s+0x([0-9A-Fa-f]{{8}})UL", source
                )
                self.assertIsNotNone(match, f"固件缺少{constant}")
                data = (ROOT / "Tools" / "bin_files" / filename).read_bytes()
                self.assertEqual(int(match.group(1), 16), zlib.crc32(data) & 0xFFFFFFFF)


if __name__ == "__main__":
    unittest.main()
