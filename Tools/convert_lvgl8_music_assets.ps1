param(
    [Parameter(Mandatory = $true)]
    [string]$SourceDirectory,

    [Parameter(Mandatory = $true)]
    [string]$OutputPath
)

$ErrorActionPreference = 'Stop'

$assetFiles = @(
    'img_lv_demo_music_wave_top.c',
    'img_lv_demo_music_wave_bottom.c',
    'img_lv_demo_music_logo.c',
    'img_lv_demo_music_corner_left.c',
    'img_lv_demo_music_corner_right.c',
    'img_lv_demo_music_btn_play.c',
    'img_lv_demo_music_btn_pause.c',
    'img_lv_demo_music_btn_prev.c',
    'img_lv_demo_music_btn_next.c',
    'img_lv_demo_music_btn_rnd.c',
    'img_lv_demo_music_btn_loop.c',
    'img_lv_demo_music_slider_knob.c',
    'img_lv_demo_music_cover_1.c',
    'img_lv_demo_music_icon_1.c',
    'img_lv_demo_music_icon_2.c',
    'img_lv_demo_music_icon_3.c',
    'img_lv_demo_music_icon_4.c',
    'img_lv_demo_music_btn_list_play.c',
    'img_lv_demo_music_btn_list_pause.c',
    'img_lv_demo_music_list_border.c'
)

function Add-ByteArray {
    param(
        [System.Text.StringBuilder]$Builder,
        [string]$Symbol,
        [byte[]]$Bytes
    )

    [void]$Builder.AppendLine("const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST uint8_t ${Symbol}_map[] = {")
    for ($offset = 0; $offset -lt $Bytes.Length; $offset += 16) {
        $count = [Math]::Min(16, $Bytes.Length - $offset)
        $items = for ($index = 0; $index -lt $count; $index++) {
            '0x{0:x2}' -f $Bytes[$offset + $index]
        }
        [void]$Builder.AppendLine(('  ' + ($items -join ',') + ','))
    }
    [void]$Builder.AppendLine('};')
    [void]$Builder.AppendLine()
}

$builder = [System.Text.StringBuilder]::new()
[void]$builder.AppendLine('#include "lvgl.h"')
[void]$builder.AppendLine()

foreach ($assetFile in $assetFiles) {
    $sourcePath = Join-Path $SourceDirectory $assetFile
    $text = Get-Content -LiteralPath $sourcePath -Raw -Encoding UTF8
    $symbol = [regex]::Match(
        $text,
        '(?:const\s+)?lv_img_dsc_t\s+(?<symbol>[A-Za-z0-9_]+)\s*=').Groups['symbol'].Value
    if ([System.IO.Path]::GetFileNameWithoutExtension($assetFile).EndsWith('_large')) {
        $symbol += '_large'
    }
    $width = [int][regex]::Match(
        $text, '\.header\.w\s*=\s*(?<value>\d+)').Groups['value'].Value
    $height = [int][regex]::Match(
        $text, '\.header\.h\s*=\s*(?<value>\d+)').Groups['value'].Value
    $hasAlpha = $text.Contains('LV_IMG_CF_TRUE_COLOR_ALPHA')
    $section = [regex]::Match(
        $text,
        '#if LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP == 0(?<body>.*?)#endif',
        [System.Text.RegularExpressions.RegexOptions]::Singleline).Groups['body'].Value

    if (($symbol.Length -eq 0) -or ($width -le 0) -or
        ($height -le 0) -or ($section.Length -eq 0)) {
        throw "Invalid LVGL 8 asset: $sourcePath"
    }
    $matches = [regex]::Matches($section, '0x(?<value>[0-9a-fA-F]{2})')
    $sourceBytes = [byte[]]::new($matches.Count)
    for ($index = 0; $index -lt $matches.Count; $index++) {
        $sourceBytes[$index] = [Convert]::ToByte(
            $matches[$index].Groups['value'].Value, 16)
    }

    $pixelCount = $width * $height
    if ($hasAlpha) {
        if ($sourceBytes.Length -ne ($pixelCount * 3)) {
            throw "Unexpected alpha asset size: $assetFile"
        }
        $outputBytes = [byte[]]::new($pixelCount * 3)
        for ($pixel = 0; $pixel -lt $pixelCount; $pixel++) {
            $outputBytes[$pixel * 2] = $sourceBytes[$pixel * 3]
            $outputBytes[$pixel * 2 + 1] = $sourceBytes[$pixel * 3 + 1]
            $outputBytes[$pixelCount * 2 + $pixel] =
                $sourceBytes[$pixel * 3 + 2]
        }
        $format = 'LV_COLOR_FORMAT_RGB565A8'
    }
    else {
        if ($sourceBytes.Length -ne ($pixelCount * 2)) {
            throw "Unexpected RGB565 asset size: $assetFile"
        }
        $outputBytes = $sourceBytes
        $format = 'LV_COLOR_FORMAT_RGB565'
    }

    Add-ByteArray -Builder $builder -Symbol $symbol -Bytes $outputBytes
    [void]$builder.AppendLine("const lv_image_dsc_t $symbol = {")
    [void]$builder.AppendLine('  .header.magic = LV_IMAGE_HEADER_MAGIC,')
    [void]$builder.AppendLine("  .header.cf = $format,")
    [void]$builder.AppendLine("  .header.stride = $($width * 2),")
    [void]$builder.AppendLine('  .header.flags = 0,')
    [void]$builder.AppendLine("  .header.w = $width,")
    [void]$builder.AppendLine("  .header.h = $height,")
    [void]$builder.AppendLine("  .data_size = sizeof(${symbol}_map),")
    [void]$builder.AppendLine("  .data = ${symbol}_map,")
    [void]$builder.AppendLine('};')
    [void]$builder.AppendLine()
}

$outputDirectory = Split-Path -Parent $OutputPath
[System.IO.Directory]::CreateDirectory($outputDirectory) | Out-Null
$utf8WithoutBom = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllText($OutputPath, $builder.ToString(),
                               $utf8WithoutBom)
Write-Output "Generated $OutputPath from $($assetFiles.Count) assets"
