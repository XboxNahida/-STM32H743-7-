// 将项目内SVG转换为LVGL 9的RGB565A8只读资源；像素区与透明度区独立排列。
import fs from 'node:fs';
import path from 'node:path';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';
const require = createRequire(import.meta.url);
const sharp = require(process.env.SCOPE_SHARP_MODULE || 'sharp');
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const asset = path.join(root, 'lvgl_GUI/custom/assets');
const { data, info } = await sharp(path.join(asset, 'oscilloscope.svg'))
    .resize(80, 80).ensureAlpha().raw().toBuffer({ resolveWithObject: true });
if (info.width !== 80 || info.height !== 80 || info.channels !== 4) throw Error('图标格式无效');
const pixels = Buffer.alloc(80 * 80 * 3);
for (let i = 0; i < 6400; i++) {
    const code = ((data[i * 4] >> 3) << 11) | ((data[i * 4 + 1] >> 2) << 5) | (data[i * 4 + 2] >> 3);
    pixels.writeUInt16LE(code, i * 2);
    pixels[12800 + i] = data[i * 4 + 3];
}
const rows = [];
for (let i = 0; i < pixels.length; i += 32) {
    rows.push('    ' + [...pixels.subarray(i, i + 32)].map(v => '0x' + v.toString(16).padStart(2, '0')).join(',') + ',');
}
fs.writeFileSync(path.join(asset, 'oscilloscope_icon.h'),
`/* 由Tools/生成示波器图标.mjs生成；请修改SVG后重新转换。 */
#ifndef SCOPE_ICON_RESOURCE_H
#define SCOPE_ICON_RESOURCE_H
#include "lvgl.h"
static const LV_ATTRIBUTE_MEM_ALIGN uint8_t scope_icon_pixels[19200] = {
${rows.join('\n')}
};
static const lv_image_dsc_t scope_icon = {
    .header = {.magic=LV_IMAGE_HEADER_MAGIC, .cf=LV_COLOR_FORMAT_RGB565A8,
               .flags=0, .w=80, .h=80, .stride=160},
    .data_size=sizeof(scope_icon_pixels), .data=scope_icon_pixels
};
#endif
`, 'utf8');
await sharp(data, { raw: info }).png().toFile(path.join(root, 'tmp/示波器图标.png'));
console.log('图标已生成：80×80 RGB565A8，19200字节，只读片内常量。');
