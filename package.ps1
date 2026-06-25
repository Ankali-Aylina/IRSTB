# TemperatureControlV3
param(
    [switch]$SkipBuild,
    [string]$QtPath = "",
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$scriptDir = $PSScriptRoot
Set-Location $scriptDir

$buildDir = "$scriptDir\x64\$Configuration"
$exePath = "$buildDir\TemperatureControlV3.exe"

$versionLine = Select-String -Path "$scriptDir\ApplicationBootstrap.cpp" -Pattern 'setApplicationVersion'
$version = "unknown"
if ($versionLine -match '"([^"]+)"') { $version = $Matches[1] }
$zipName = "TemperatureControlV3_v${version}_x64.zip"
$zipPath = "$scriptDir\$zipName"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " TemperatureControlV3 发布打包" -ForegroundColor Cyan
Write-Host " 版本: $version" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# 步骤 1：编译
if (-not $SkipBuild) {
    Write-Host "[1/4] 编译 Release|x64 ..." -ForegroundColor Yellow

    $msbuild = $null
    $roots = @($env:ProgramFiles, ${env:ProgramFiles(x86)})
    foreach ($r in $roots) {
        $vsDir = Join-Path $r "Microsoft Visual Studio"
        if (-not (Test-Path $vsDir)) { continue }
        $candidates = Get-ChildItem "$vsDir\*\MSBuild\Current\Bin\MSBuild.exe" -ErrorAction SilentlyContinue
        if ($candidates) { $msbuild = $candidates[0].FullName; break }
    }
    if (-not $msbuild) { throw "找不到 MSBuild.exe" }
    Write-Host "  MSBuild: $msbuild" -ForegroundColor DarkGray

    & $msbuild "$scriptDir\TemperatureControlV3.sln" /p:Configuration=$Configuration /p:Platform=x64 /m /v:minimal
    if ($LASTEXITCODE -ne 0) { throw "编译失败" }
    Write-Host "  编译完成" -ForegroundColor Green
}
else {
    Write-Host "[1/4] 跳过编译 (-SkipBuild)" -ForegroundColor Yellow
}

if (-not (Test-Path $exePath)) { throw "找不到 $exePath" }

# 步骤 2：windeployqt
Write-Host "[2/4] windeployqt ..." -ForegroundColor Yellow

if (-not $QtPath) {
    $qtFound = Get-ChildItem "C:\Qt\*\msvc*\bin\windeployqt.exe" -ErrorAction SilentlyContinue | Sort-Object -Descending
    if (-not $qtFound) { $qtFound = Get-ChildItem "D:\Qt\*\msvc*\bin\windeployqt.exe" -ErrorAction SilentlyContinue | Sort-Object -Descending }
    if ($qtFound) { $windeployqt = $qtFound[0].FullName }
    else { throw "找不到 windeployqt，请用 -QtPath 指定" }
}
else { $windeployqt = "$QtPath\bin\windeployqt.exe" }
Write-Host "  $windeployqt" -ForegroundColor DarkGray

Push-Location $buildDir
& $windeployqt "TemperatureControlV3.exe" --no-translations
if ($LASTEXITCODE -ne 0) { throw "windeployqt 失败" }
Pop-Location
Write-Host "  windeployqt 完成" -ForegroundColor Green

# 步骤 3：精简
Write-Host "[3/4] 精简 ..." -ForegroundColor Yellow

if (Test-Path "$buildDir\generic") { Remove-Item "$buildDir\generic" -Recurse -Force }
@("qgif.dll","qjpeg.dll") | ForEach-Object {
    $p = "$buildDir\imageformats\$_"
    if (Test-Path $p) { Remove-Item $p -Force }
}
$td = "$buildDir\translations"
if (Test-Path $td) {
    Get-ChildItem "$td\*.qm" | Where-Object { $_.Name -ne "qt_zh_CN.qm" } | Remove-Item -Force
}
@("app.log","WinRing0x64.sys","WinRing0x64.dll") | ForEach-Object {
    $p = "$buildDir\$_"
    if (Test-Path $p) { Remove-Item $p -Force }
}
Get-ChildItem "$buildDir\*.lib" -ErrorAction SilentlyContinue | Remove-Item -Force
Write-Host "  精简完成" -ForegroundColor Green

# 步骤 4：打包
Write-Host "[4/4] 打包 ..." -ForegroundColor Yellow
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Compress-Archive -Path "$buildDir\*" -DestinationPath $zipPath

$sizeMB = [math]::Round((Get-Item $zipPath).Length / 1MB, 1)

# 步骤 4.5：生成 SHA256 校验文件
Write-Host "[4.5/4] 生成 SHA256 校验文件 ..." -ForegroundColor Yellow
$hash = (Get-FileHash -Path $zipPath -Algorithm SHA256).Hash
$hashFile = "$zipPath.sha256"
"$hash  $zipName" | Out-File -FilePath $hashFile -Encoding ASCII
Write-Host "  SHA256: $hash" -ForegroundColor DarkGray

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " 打包完成!  大小: ${sizeMB} MB" -ForegroundColor Green
Write-Host " 文件: $zipName" -ForegroundColor White
Write-Host " SHA256: $hash" -ForegroundColor White
Write-Host ""
Write-Host " 提醒: 用户需安装 VC Redist x64" -ForegroundColor Yellow
Write-Host " https://aka.ms/vs/17/release/vc_redist.x64.exe" -ForegroundColor DarkGray
Write-Host "========================================" -ForegroundColor Cyan
