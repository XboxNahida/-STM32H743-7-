<#
.SYNOPSIS
    LVGL TTF -> BIN converter with split-size charset
.DESCRIPTION
    - Sizes in $FullCJKSizes get full CJK range (0x4E00-0x9FFF)
    - Other sizes get exact UI charset from charset_ui.txt (auto-scanned)
    - ASCII + symbols always included for all sizes
#>

param(
    [string]$FontPath = ".\myh_UI.ttf",
    [int[]]$Sizes = @(12, 14, 16, 18, 20, 30),
    [int[]]$FullCJKSizes = @(20),
    [ValidateSet(1, 2, 3, 4, 8)]
    [int]$Bpp = 4,
    [string]$OutputDir = ".\bin_files",
    [string]$FontName = "myh_ui",
    [string]$CharsetFile = ".\charset_ui.txt"
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$LvFontConv = Join-Path $ScriptDir "lv_font_conv-master\lv_font_conv.js"

if (-not (Test-Path $LvFontConv)) { Write-Error "Cannot find lv_font_conv.js"; exit 1 }
if (-not (Test-Path $FontPath))  { Write-Error "Cannot find font file"; exit 1 }
if (-not (Test-Path $OutputDir)) { New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null }

# ---- Common ranges for ALL sizes ----
$CommonRanges = @(
    "0x20-0xFF",          # ASCII + Latin
    "0x3000-0x303F",      # CJK punctuation
    "0xFE30-0xFE6D",      # CJK compatibility forms
    "0xFF01-0xFF5E",      # Fullwidth ASCII
    "0xFFE0-0xFFE6",      # Fullwidth symbols
    "0x2460-0x2473",      # circled numbers 1-20
    "0x2605-0x260A"       # star / phone / weather
)

# ---- Full CJK range for large-size fonts ----
$FullCJKRange = "0x4E00-0x9FFF"

# ---- Exact UI charset for small-size fonts ----
$Symbols = ""
$CharsetPath = Join-Path $ScriptDir $CharsetFile
if (Test-Path $CharsetPath) {
    $Symbols = (Get-Content $CharsetPath -Raw).Trim()
    Write-Host "UI charset : $($Symbols.Length) CJK chars from $CharsetFile"
} else {
    $guiDir = Join-Path (Split-Path $ScriptDir -Parent) "lvgl_GUI"
    $coreDir = Join-Path (Split-Path $ScriptDir -Parent) "Core"
    $all = @{}
    foreach ($dir in @($guiDir, $coreDir)) {
        Get-ChildItem -Path $dir -Recurse -Include "*.c","*.h" -ErrorAction SilentlyContinue |
            Where-Object { -not ($_.Name -like "lv_font_*") } |
            ForEach-Object {
                $txt = Get-Content $_.FullName -Raw -Encoding UTF8 -ErrorAction SilentlyContinue
                if ($txt) {
                    [regex]::Matches($txt, '[\u4e00-\u9fff]') | ForEach-Object { $all[$_.Value] = $true }
                }
            }
    }
    $Symbols = [string]::Join('', ($all.Keys | Sort-Object))
    $Symbols | Out-File -FilePath $CharsetPath -Encoding UTF8 -NoNewline
    Write-Host "UI charset : $($Symbols.Count) CJK chars (auto-scanned -> $CharsetFile)"
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " LVGL TTF -> BIN (split-size charset)" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Font     : $FontPath"
Write-Host "Sizes    : $($Sizes -join ', ')"
Write-Host "Full CJK : $($FullCJKSizes -join ', ') (0x4E00-0x9FFF)"
Write-Host "Exact UI : other sizes (${($Symbols.Length)} chars)"
Write-Host ""

$Current = 0
foreach ($size in $Sizes) {
    $Current++
    $OutputFile = Join-Path $OutputDir "lv_font_${FontName}_${size}.bin"

    $RangeArgs = @()
    foreach ($r in $CommonRanges) { $RangeArgs += "-r"; $RangeArgs += $r }

    # Decide charset strategy
    if ($FullCJKSizes -contains $size) {
        $label = "FULL CJK"
        $RangeArgs += "-r"; $RangeArgs += $FullCJKRange
    } else {
        $label = "exact UI"
        $RangeArgs += "--symbols"; $RangeArgs += $Symbols
    }

    Write-Host "[$Current/$($Sizes.Count)] ${size}px ($label) ..." -ForegroundColor Yellow

    & node $LvFontConv --size $size --bpp $Bpp --format bin --font $FontPath --no-compress -o $OutputFile @RangeArgs 2>&1 | Out-Null

    if ($LASTEXITCODE -eq 0) {
        $kb = [math]::Round((Get-Item $OutputFile).Length / 1024, 1)
        Write-Host "  [OK] $kb KB" -ForegroundColor Green
    } else {
        Write-Host "  [ERROR]" -ForegroundColor Red
    }
}
Write-Host "`nDone!" -ForegroundColor Green
