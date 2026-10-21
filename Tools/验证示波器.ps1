param([string]$Compiler = 'C:/msys64/ucrt64/bin/gcc.exe')

$ErrorActionPreference = 'Stop'
$scopeRoot = Split-Path -Parent $PSScriptRoot
$scopeOutput = Join-Path $scopeRoot 'tmp/scope_validation'
New-Item -ItemType Directory -Path $scopeOutput -Force | Out-Null
if (!(Test-Path -LiteralPath $Compiler)) { throw "找不到 GCC：$Compiler" }

# 只构建主机测试，不烧录、不访问真实ADC/DAC，不修改原有源码。
$scopeCases = @(
    @{ Name='scope_phy'; Args=@('Tests/host/test_scope_phy.c', 'Core/Peripheral/Src/oscilloscope_phy.c') },
    @{ Name='oscilloscope_workspace'; Args=@('Tests/host/test_oscilloscope_workspace.c') },
    @{ Name='oscilloscope_zoom'; Args=@('-ICore/Application/Inc', 'Tests/host/test_oscilloscope_zoom.c', 'Core/Application/Src/oscilloscope_zoom.c', '-lm') },
    @{ Name='oscilloscope_core'; Args=@('-ICore/Application/Inc', 'Tests/host/test_oscilloscope_core.c', 'Core/Application/Src/oscilloscope_core.c', '-lm') },
    @{ Name='oscilloscope_timing'; Args=@('-ICore/Peripheral/Inc', 'Tests/host/test_oscilloscope_timing.c', 'Core/Peripheral/Src/oscilloscope_timing.c') },
    @{ Name='oscilloscope_driver'; Args=@('-ITests/host/scope_hal', '-ICore/Peripheral/Inc', 'Tests/host/test_oscilloscope_driver.c', 'Core/Peripheral/Src/oscilloscope_timing.c') },
    @{ Name='oscilloscope_session'; Args=@('-ICore/Application/Inc', '-ICore/Peripheral/Inc', 'Tests/host/test_oscilloscope_session.c', 'Core/Application/Src/oscilloscope_session.c', 'Core/Application/Src/oscilloscope_core.c', '-lm') },
    @{ Name='oscilloscope_storage'; Args=@('-ICore/Application/Inc', '-ICore/Peripheral/Inc', 'Tests/host/test_oscilloscope_storage.c', 'Core/Application/Src/oscilloscope_storage.c', '-lm') },
    @{ Name='oscilloscope_accurate'; Args=@('-ICore/Application/Inc', 'Tests/host/test_oscilloscope_accurate.c', 'Core/Application/Src/oscilloscope_accurate.c', 'Core/Application/Src/oscilloscope_core.c', 'Core/Application/Src/scope_reference_dsp.c', 'Core/Application/Src/scope_reference_frequency.c', '-lm') },
    @{ Name='adc_measurement_service'; Args=@('-DADC_MEASUREMENT_HOST_TEST', '-ICore/Peripheral/Inc', 'Tests/host/test_adc_measurement_service.c', 'Core/Peripheral/Src/adc_measurement_service.c') },
    @{ Name='adc_mapping_contract'; Args=@('Tests/host/test_adc_mapping_contract.c') },
    @{ Name='resource_manager'; Args=@('-ICore/Peripheral/Inc', 'Tests/host/test_resource_manager.c', 'Core/Peripheral/Src/resource_manager.c') }
)

Push-Location $scopeRoot
try {
    foreach ($scopeCase in $scopeCases) {
        $scopeExe = Join-Path $scopeOutput ($scopeCase.Name + '.exe')
        $scopeArgs = @('-std=c11', '-Wall', '-Wextra', '-Werror', '-fshort-enums', '-DSCOPE_HOST_TEST', '-ICore/Application/Inc', '-ICore/Peripheral/Inc', 'Core/Application/Src/oscilloscope_workspace.c', 'Core/Peripheral/Src/shared_buffer.c') + $scopeCase.Args + @('-o', $scopeExe)
        & $Compiler @scopeArgs
        if ($LASTEXITCODE -ne 0) { throw "编译失败：$($scopeCase.Name)" }
        & $scopeExe
        if ($LASTEXITCODE -ne 0) { throw "测试失败：$($scopeCase.Name)" }
    }
    Write-Output "已通过 $($scopeCases.Count) 项主机测试（严格警告、短枚举）；仍需Keil目标构建和实机验收。"
}
finally { Pop-Location }
