param([string]$Compiler = 'C:/msys64/ucrt64/bin/gcc.exe',[switch]$Render)
$ErrorActionPreference='Stop'
$scopeRoot=Split-Path -Parent $PSScriptRoot
$scopeOut=Join-Path $scopeRoot 'tmp/scope_ui_validation'
New-Item -ItemType Directory -Path $scopeOut -Force | Out-Null
# 独立主机配置，使用实际LVGL源码和手写示波器模块，不改板端lv_conf.h。
$scopeCommon=@('-std=c11','-O0','-fshort-enums','-DSCOPE_HOST_TEST','-DLV_CONF_SKIP',
    '-DLV_FONT_FMT_TXT_LARGE=1',
    '-DLV_USE_STDLIB_MALLOC=1','-DLV_USE_STDLIB_STRING=1','-DLV_USE_STDLIB_SPRINTF=1',
    '-DLV_LVGL_H_INCLUDE_SIMPLE','-Dlv_font_myyh_20=lv_font_montserrat_14',
    '-Dlv_font_myyh_16=lv_font_montserrat_14',
    '-Ilvgl','-Ilvgl/porting','-Ilvgl_GUI/custom','-Ilvgl_GUI/generated',
    '-ICore/Application/Inc','-ICore/Peripheral/Inc','-ffunction-sections','-fdata-sections')
Push-Location $scopeRoot
try {
    $scopeLvglObject=Join-Path $scopeOut 'lvgl-host-large.o'
    # OpenGL着色器资源含C++原始字符串，纯C/无GPU的主机测试不需要它。
    $scopeFiles=@(rg --files lvgl/src -g '*.c' | Where-Object {$_ -notmatch '[/\\](opengles|gltf_view)[/\\]assets[/\\]'})
    $scopeNewest=(Get-ChildItem -LiteralPath lvgl/src -Recurse -File | Measure-Object LastWriteTimeUtc -Maximum).Maximum
    if(!(Test-Path -LiteralPath $scopeLvglObject) -or
       (Get-Item -LiteralPath $scopeLvglObject).LastWriteTimeUtc -lt $scopeNewest) {
        # 合并编译列表，避免一次命令行超过Windows长度上限；输出均在tmp内。
        $scopeObjects=@()
        foreach($scopeSource in $scopeFiles){
            $scopeObj=Join-Path $scopeOut (($scopeSource -replace '[/\\]','_')+'.o')
            & $Compiler @scopeCommon -c $scopeSource -o $scopeObj
            if($LASTEXITCODE -ne 0){throw "LVGL主机编译失败：$scopeSource"}
            $scopeObjects+=$scopeObj
        }
        $scopeResponse=Join-Path $scopeOut 'lvgl-link.rsp'
        [IO.File]::WriteAllLines($scopeResponse,($scopeObjects | ForEach-Object {'"'+($_ -replace '\\','/')+'"'}),[Text.UTF8Encoding]::new($false))
        & $Compiler -r ('@'+$scopeResponse) -o $scopeLvglObject
        if($LASTEXITCODE -ne 0){throw 'LVGL主机链接失败'}
    }
    $scopeSources=@('Tests/host/test_oscilloscope_ui.c','lvgl_GUI/custom/oscilloscope_ui.c',
        'lvgl_GUI/custom/oscilloscope_controls.c','Core/Application/Src/oscilloscope_zoom.c',
        'Core/Application/Src/oscilloscope_session.c','Core/Application/Src/oscilloscope_core.c',
        'Core/Application/Src/oscilloscope_storage.c','Core/Application/Src/oscilloscope_accurate.c',
        'Core/Application/Src/scope_reference_dsp.c','Core/Application/Src/scope_reference_frequency.c',
        'Core/Application/Src/oscilloscope_workspace.c','Core/Peripheral/Src/shared_buffer.c')
    $scopeExe=Join-Path $scopeOut 'oscilloscope_ui.exe'
    if($Render){
        # 渲染使用项目原始中文字形，避免ASCII替身掩盖中文截断；不修改板端字体桥。
        $scopeCommon=@($scopeCommon | Where-Object {$_ -notmatch '^-Dlv_font_myyh_'})
        $scopeSources+=@('Tools/c_files/lv_font_myyh_16.c','Tools/c_files/lv_font_myyh_20.c')
    }
    & $Compiler @scopeCommon -Wall -Wextra -Werror @scopeSources $scopeLvglObject '-Wl,--gc-sections' '-Wl,--wrap=lv_draw_label' -lm -o $scopeExe
    if($LASTEXITCODE -ne 0){throw '示波器界面主机编译失败'}
    if($Render){$env:SCOPE_RENDER='1'}
    try { & $scopeExe } finally { if($Render){Remove-Item Env:SCOPE_RENDER -ErrorAction SilentlyContinue} }
    if($LASTEXITCODE -ne 0){throw '示波器界面集成测试失败'}
}
finally {Pop-Location}
