; TemperatureControlV3 Inno Setup 安装脚本
; 用法：
;   1. 先运行 .\package.ps1 -SkipBuild 生成 Release 目录
;   2. 用 Inno Setup 打开此文件，点击"编译"即可生成安装包

#define MyAppName     "TemperatureControlV3"
#define MyAppVersion  "3.4.0.0"
#define MyAppExeName  "TemperatureControlV3.exe"
#define MyAppPublisher "Ankali-Aylina"
#define MyAppURL      ""
#define MyAppSource   "x64\Release"

[Setup]
AppId={{D2AC4FC9-8819-4172-AB93-5C2129504C89}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir=installer
OutputBaseFilename={#MyAppName}_v{#MyAppVersion}_Setup
SetupIconFile=TreeNetWork.ico
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
DisableProgramGroupPage=auto
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}
WizardStyle=modern

[Languages]
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "附加快捷方式"

[Files]
; 主程序和 Qt DLL
Source: "{#MyAppSource}\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppSource}\Qt6Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppSource}\Qt6Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppSource}\Qt6Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppSource}\Qt6Network.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppSource}\Qt6Svg.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppSource}\D3Dcompiler_47.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppSource}\opengl32sw.dll"; DestDir: "{app}"; Flags: ignoreversion

; Qt 插件目录
Source: "{#MyAppSource}\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#MyAppSource}\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#MyAppSource}\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#MyAppSource}\iconengines\*"; DestDir: "{app}\iconengines"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#MyAppSource}\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#MyAppSource}\tls\*"; DestDir: "{app}\tls"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#MyAppSource}\translations\*"; DestDir: "{app}\translations"; Flags: ignoreversion recursesubdirs createallsubdirs

; PawnIO 驱动安装程序
Source: "res\lib\PawnIO_setup.exe"; DestDir: "{app}"; Flags: ignoreversion

; AMD RyzenMaster 驱动文件（amd_dll.dll 内部依赖，必须放在 bin\ 子目录）
Source: "res\lib\bin\AMDRyzenMasterDriver.sys"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "res\lib\bin\AMDRyzenMasterDriver.inf"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "res\lib\bin\AMDRyzenMasterDriver.cat"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "res\lib\bin\Device.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "res\lib\bin\Platform.dll"; DestDir: "{app}\bin"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\卸载 {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
; 卸载时清理开机自启注册表项
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueName: "TemperatureControlV3"; Flags: uninsdeletevalue

[Run]
; PawnIO 驱动安装（仅在未安装时执行）
Filename: "{app}\PawnIO_setup.exe"; Parameters: "-install"; StatusMsg: "正在安装 PawnIO 驱动..."; Flags: runhidden waituntilterminated; Check: not IsPawnIODriverInstalled
Filename: "{app}\{#MyAppExeName}"; Description: "启动 {#MyAppName}"; Flags: postinstall nowait skipifsilent runascurrentuser

[UninstallDelete]
Type: files; Name: "{app}\app.log"
Type: files; Name: "{app}\app_*.log"
Type: files; Name: "{app}\config.ini"

[Code]
function IsPawnIODriverInstalled: Boolean;
var
  Version: String;
begin
  Result := RegQueryStringValue(HKLM, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\PawnIO', 'DisplayVersion', Version);
end;

function IsVCppRedistInstalled: Boolean;
var
  Version: Cardinal;
begin
  Result := RegQueryDWordValue(HKLM, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64', 'Installed', Version)
         or RegQueryDWordValue(HKLM, 'SOFTWARE\WOW6432Node\Microsoft\VisualStudio\14.0\VC\Runtimes\x64', 'Installed', Version);
end;

function InitializeSetup: Boolean;
var
  ErrorCode: Integer;
begin
  if not IsVCppRedistInstalled then
  begin
    if MsgBox('检测到未安装 Visual C++ 运行时库。' + #13#10#13#10 +
              '是否前往微软官网下载？' + #13#10 +
              '(安装程序将继续，但程序可能无法正常运行。)',
              mbConfirmation, MB_YESNO) = IDYES then
    begin
      ShellExec('open', 'https://aka.ms/vs/17/release/vc_redist.x64.exe', '', '', SW_SHOW, ewNoWait, ErrorCode);
    end;
  end;
  Result := True;
end;

function InitializeUninstall: Boolean;
var
  ErrorCode: Integer;
  AppWnd: HWND;
begin
  Result := True;

  // 先尝试通过窗口标题查找（正常显示状态）
  AppWnd := FindWindowByWindowName('TCV3');

  if AppWnd <> 0 then
  begin
    if MsgBox('{#MyAppName} 正在运行，需要关闭后才能继续卸载。' + #13#10#13#10 +
              '是否关闭程序并继续卸载？',
              mbConfirmation, MB_YESNO) = IDYES then
    begin
      PostMessage(AppWnd, $0010 {WM_CLOSE}, 0, 0);
      Sleep(1000);
    end
    else
    begin
      Result := False;
      Exit;
    end;
  end;

  // 兜底：强制终止所有同名进程（处理托盘最小化、后台运行等窗口不可见的情况）
  Exec('taskkill', '/F /IM TemperatureControlV3.exe', '', SW_HIDE, ewWaitUntilTerminated, ErrorCode);
  Sleep(500);

  // 最终检查，确保进程已完全退出
  if FindWindowByWindowName('TCV3') <> 0 then
  begin
    MsgBox('无法关闭 {#MyAppName}，请手动关闭后重试。', mbError, MB_OK);
    Result := False;
  end;
end;
