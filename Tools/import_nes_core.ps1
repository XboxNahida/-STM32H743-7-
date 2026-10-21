param(
    [Parameter(Mandatory = $true)]
    [string]$SourceRoot,
    [string]$DestinationRoot = (Join-Path $PSScriptRoot '..\Core\Middlewares\NES'),
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$resolvedSourcePath = (Resolve-Path -LiteralPath $SourceRoot).ProviderPath
$workspace = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$destination = [IO.Path]::GetFullPath($DestinationRoot)

if (-not $destination.StartsWith($workspace, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Destination must stay inside workspace: $destination"
}
if ((Test-Path -LiteralPath $destination) -and -not $Force) {
    $existing = Get-ChildItem -LiteralPath $destination -Force -ErrorAction SilentlyContinue
    if ($null -ne $existing) {
        throw "Destination is not empty; use -Force only for intentional re-import"
    }
}

$coreFiles = @(
    '6502.s',
    'nes_main.c', 'nes_main.h',
    'nes_ppu.c', 'nes_ppu.h',
    'nes_apu.c', 'nes_apu.h',
    'nes_mapper.c', 'nes_mapper.h',
    'nes_crc32.c'
)
$mapperFiles = @(
    '000.cpp', '003.cpp', '004.cpp', '023.cpp'
)

$cp936 = [Text.Encoding]::GetEncoding(936)
$utf8 = New-Object Text.UTF8Encoding($false, $true)

New-Item -ItemType Directory -Force -Path $destination | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $destination 'mapper') | Out-Null

foreach ($relative in $coreFiles) {
    $inputPath = Join-Path $resolvedSourcePath $relative
    $outputPath = Join-Path $destination $relative
    $text = $cp936.GetString([IO.File]::ReadAllBytes($inputPath))
    [IO.File]::WriteAllBytes($outputPath, $utf8.GetBytes($text))
}
foreach ($relative in $mapperFiles) {
    $inputPath = Join-Path (Join-Path $resolvedSourcePath 'mapper') $relative
    $outputPath = Join-Path (Join-Path $destination 'mapper') $relative
    $text = $cp936.GetString([IO.File]::ReadAllBytes($inputPath))
    [IO.File]::WriteAllBytes($outputPath, $utf8.GetBytes($text))
}

Write-Output ("IMPORTED core={0} mapper={1} destination={2}" -f `
    $coreFiles.Count, $mapperFiles.Count, $destination)
