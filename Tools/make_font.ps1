<#
.SYNOPSIS
    LVGL lv_font_conv wrapper — single size, merged CJK + Font Awesome
#>
param(
    [Parameter(Mandatory)] [int]$Size,
    [ValidateSet("full","ui")] [string]$Mode = "ui",
    [int]$Bpp = 4,
    [string]$FontPath = ".\myh_UI.ttf",
    [string]$FaFont = "fa-solid-900.otf",
    [string]$OutDir = ".\bin_files",
    [string]$FontName = "myh_ui"
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$LvFontConv = Join-Path $ScriptDir "lv_font_conv-master\lv_font_conv.js"
$FaFontPath = Join-Path $ScriptDir $FaFont
$Output = Join-Path $ScriptDir "$OutDir\lv_font_${FontName}_${Size}.bin"

# Build args: --font CJK ...ranges... --font FA ...ranges...
$Args = @("--size", $Size, "--bpp", $Bpp, "--format", "bin", "--no-compress", "-o", $Output)

# Font 1: CJK
$Args += "--font"; $Args += $FontPath
$Args += "-r"; $Args += "0x20-0xFF"
$Args += "-r"; $Args += "0x3000-0x303F"
$Args += "-r"; $Args += "0xFF01-0xFF5E"
$Args += "-r"; $Args += "0xFFE0-0xFFE6"
$Args += "-r"; $Args += "0x2460-0x2473"
$Args += "-r"; $Args += "0x2605-0x260A"

if ($Mode -eq "full") {
    $Args += "-r"; $Args += "0x4E00-0x9FFF"
    Write-Host "${Size}px FULL CJK + FA -> $Output" -ForegroundColor Yellow
} else {
    $CharsetFile = Join-Path $ScriptDir "charset_ui.txt"
    if (-not (Test-Path $CharsetFile)) { Write-Error "charset_ui.txt not found"; exit 1 }
    $Symbols = (Get-Content $CharsetFile -Raw).Trim()
    $Args += "--symbols"; $Args += $Symbols
    Write-Host "${Size}px exact UI ($($Symbols.Length) chars) + FA -> $Output" -ForegroundColor Yellow
}

# Font 2: Font Awesome icons
$Args += "--font"; $Args += $FaFontPath
$Args += "-r"; $Args += "0xF000-0xF2FF"

& node $LvFontConv @Args 2>&1
if ($LASTEXITCODE -eq 0) {
    $kb = [math]::Round((Get-Item $Output).Length / 1024, 1)
    Write-Host "  [OK] $kb KB" -ForegroundColor Green
} else {
    Write-Host "  [ERROR]" -ForegroundColor Red
}
