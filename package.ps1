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

$versionLine = Select-String -Path "$scriptDir\resource.h" -Pattern 'APP_VERSION_STR'
$version = "unknown"
if ($versionLine -match '"([^"]+)"') { $version = $Matches[1] }
$zipName = "TemperatureControlV3_v${version}_x64.zip"
# zip 输出到 installer\ 文件夹（与 Inno 安装包同目录）
$zipDir = "$scriptDir\installer"
New-Item -ItemType Directory -Force -Path $zipDir | Out-Null
$zipPath = "$zipDir\$zipName"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " TemperatureControlV3 发布打包" -ForegroundColor Cyan
Write-Host " 版本: $version" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# 步骤 1：编译
if (-not $SkipBuild) {
    Write-Host "[1/4] 编译 Release|x64 ..." -ForegroundColor Yellow
    $msbuild = $null
    # 优先用 vswhere（VS 官方定位工具，兼容所有安装位置/版本/Preview/Build Tools）
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild `
            -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
    }
    # 兜底：遍历常见目录（注意 VS 实际布局是 2022\Community\MSBuild 两层）
    if (-not $msbuild) {
        $roots = @($env:ProgramFiles, ${env:ProgramFiles(x86)})
        foreach ($r in $roots) {
            $vsDir = Join-Path $r "Microsoft Visual Studio"
            if (-not (Test-Path $vsDir)) { continue }
            $candidates = Get-ChildItem "$vsDir\*\*\MSBuild\Current\Bin\MSBuild.exe" -ErrorAction SilentlyContinue
            if ($candidates) { $msbuild = $candidates[0].FullName; break }
        }
    }
    if (-not $msbuild) { throw "找不到 MSBuild.exe（请确认已安装 VS2022 或 Build Tools）" }
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
# --qmldir 必须指定，否则 QML 引擎找不到 QtQuick 运行时插件
& $windeployqt "TemperatureControlV3.exe" --no-translations --qmldir "$scriptDir\qml"
if ($LASTEXITCODE -ne 0) { throw "windeployqt 失败" }
Pop-Location
Write-Host "  windeployqt 完成" -ForegroundColor Green

# 步骤 2.5：校验 QML 模块没有被 installer.iss 漏掉
# 背景：installer.iss 的 [Files] 用 Excludes 精简掉用不到的 QML 模块（控制安装包体积）。
#       但这份列表是手工维护的 —— 一旦 QML 新增了 import 而列表没同步，
#       打包过程一切正常，装完却是界面直接起不来（"module QtQuick.xxx is not installed"）。
#       v4.5.0.0 的 QtQuick\Shapes 就是这么漏的，这里补一道交叉校验。
Write-Host "[2.5/4] 校验 QML 模块 ..." -ForegroundColor Yellow
$issPath = "$scriptDir\installer.iss"
if (Test-Path $issPath) {
    $excluded = @()
    if ((Get-Content $issPath -Raw) -match 'Excludes:\s*"([^"]*)"') {
        $excluded = @($Matches[1] -split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ })
    }
    # 收集 qml/ 下所有 import 的 Qt 模块
    $mods = @{}
    Get-ChildItem "$scriptDir\qml" -Filter *.qml | ForEach-Object {
        Select-String -Path $_.FullName -Pattern '^\s*import\s+(Qt[A-Za-z0-9_.]+)\s*[0-9.]*\s*$' |
            ForEach-Object { $mods[$_.Matches[0].Groups[1].Value] = $true }
    }
    $problems = @()
    foreach ($m in ($mods.Keys | Sort-Object)) {
        $rel = $m -replace '\.', '\'
        if (-not (Test-Path "$buildDir\qml\$rel\qmldir")) {
            $problems += "$m —— 未部署到 $buildDir\qml\$rel\"
            continue
        }
        foreach ($ex in $excluded) {
            if ($ex -ne 'qmltooling' -and $rel.StartsWith($ex, [System.StringComparison]::OrdinalIgnoreCase)) {
                $problems += "$m —— 被 installer.iss 的 Excludes 排除（$ex），但界面用到了"
            }
        }
    }
    if ($problems.Count -gt 0) {
        throw ("以下 QML 模块会缺失，装完后界面将无法加载：`n    " + (($problems | Sort-Object -Unique) -join "`n    ") +
               "`n  请修改 installer.iss 的 Excludes 后重试。")
    }
    Write-Host ("  已校验 {0} 个 QML 模块，均可用" -f $mods.Count) -ForegroundColor Green
}

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
