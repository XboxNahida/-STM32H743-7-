param(
    [Parameter(Mandatory = $true)]
    [string]$InputPath,

    [Parameter(Mandatory = $true)]
    [string]$OutputPath,

    [ValidateRange(1, 1024)]
    [int]$Width = 80,

    [ValidateRange(1, 1024)]
    [int]$Height = 80,

    [string]$Symbol = "music_icon_80"
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

$source = [System.Drawing.Image]::FromFile($InputPath)
$bitmap = New-Object System.Drawing.Bitmap -ArgumentList @(
    $Width,
    $Height,
    [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)

# Match GUI Guider assets: transparent background and high-quality resize.
    $graphics.Clear([System.Drawing.Color]::Transparent)
    $graphics.CompositingMode =
        [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
    $graphics.CompositingQuality =
        [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
    $graphics.InterpolationMode =
        [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $graphics.PixelOffsetMode =
        [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $graphics.DrawImage($source, 0, 0, $Width, $Height)

    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add('#include "lvgl.h"')
    $lines.Add('')
    $lines.Add('#ifndef LV_ATTRIBUTE_MEM_ALIGN')
    $lines.Add('#define LV_ATTRIBUTE_MEM_ALIGN')
    $lines.Add('#endif')
    $lines.Add('')
    $lines.Add("const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST uint8_t ${Symbol}_map[] = {")

    # LV_COLOR_FORMAT_RGB565A8 stores the RGB565 plane before the A8 plane.
    for ($y = 0; $y -lt $Height; $y++) {
        $bytes = New-Object System.Collections.Generic.List[string]
        for ($x = 0; $x -lt $Width; $x++) {
            $pixel = $bitmap.GetPixel($x, $y)
            $rgb565 = ((($pixel.R -shr 3) -shl 11) -bor (($pixel.G -shr 2) -shl 5) -bor ($pixel.B -shr 3))
            $bytes.Add(('0x{0:x2}' -f ($rgb565 -band 0xff)))
            $bytes.Add(('0x{0:x2}' -f (($rgb565 -shr 8) -band 0xff)))
        }
        $lines.Add('  ' + ($bytes -join ',') + ',')
    }
    for ($y = 0; $y -lt $Height; $y++) {
        $bytes = New-Object System.Collections.Generic.List[string]
        for ($x = 0; $x -lt $Width; $x++) {
            $pixel = $bitmap.GetPixel($x, $y)
            $bytes.Add(('0x{0:x2}' -f $pixel.A))
        }
        $lines.Add('  ' + ($bytes -join ',') + ',')
    }

    $lines.Add('};')
    $lines.Add('')
    $lines.Add("const lv_image_dsc_t $Symbol = {")
    $lines.Add('  .header.magic = LV_IMAGE_HEADER_MAGIC,')
    $lines.Add('  .header.cf = LV_COLOR_FORMAT_RGB565A8,')
    $lines.Add("  .header.stride = $($Width * 2),")
    $lines.Add('  .header.flags = 0,')
    $lines.Add("  .header.w = $Width,")
    $lines.Add("  .header.h = $Height,")
    $lines.Add("  .data_size = sizeof(${Symbol}_map),")
    $lines.Add("  .data = ${Symbol}_map,")
    $lines.Add('};')

    $encoding = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllLines($OutputPath, $lines, $encoding)
$graphics.Dispose()
$bitmap.Dispose()
$source.Dispose()
