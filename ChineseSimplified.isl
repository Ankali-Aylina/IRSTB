; *** Inno Setup version 6.5.0+ Chinese Simplified messages ***
;
; Simplified Chinese translation by Inno Setup Community
; Last modification date: 2025-06-25
;
[LangOptions]
LanguageName=简体中文
LanguageID=$0804
LanguageCodePage=65001
; If the language you are translating to requires special font faces or
; sizes, uncomment any of the following entries and change them accordingly.
;DialogFontName=
;DialogFontSize=9
;DialogFontBaseScaleWidth=7
;DialogFontBaseScaleHeight=15
;WelcomeFontName=Segoe UI
;WelcomeFontSize=14

[Messages]

; *** Application titles
SetupAppTitle=安装
SetupWindowTitle=安装 %1
UninstallAppTitle=卸载
UninstallAppFullTitle=%1 卸载

; *** Misc. common
InformationTitle=信息
ConfirmTitle=确认
ErrorTitle=错误

; *** SetupLdr messages
SetupLdrStartupMessage=本程序将在您的计算机上安装 %1。是否继续？
LdrCannotCreateTemp=无法创建临时文件。安装程序已中止
LdrCannotExecTemp=无法执行临时目录中的文件。安装程序已中止

; *** Startup error messages
LastErrorMessage=%1.%n%n错误 %2: %3
SetupFileMissing=安装目录中缺少文件 %1。请修正问题或获取程序的新版本。
SetupFileCorrupt=安装文件已损坏。
SetupFileCorruptOrWrongVer=安装文件已损坏，或与此版本的安装程序不兼容。请修正问题或获取程序的新版本。
InvalidParameter=命令行中指定的参数无效:%n%n%1
SetupAlreadyRunning=安装程序已在运行中。
WindowsVersionNotSupported=此程序不支持您的计算机正在运行的 Windows 版本。
WindowsServicePackRequired=此程序需要 %1 Service Pack %2 或更高版本。
NotOnThisPlatform=此程序无法在 %1 上运行。
OnlyOnThisPlatform=此程序只能在 %1 上运行。
OnlyOnTheseArchitectures=此程序只能在以下 Windows 处理器架构上安装:%n%n%1
WinVersionTooLowError=此程序需要 %1 版本 %2 或更高版本。
WinVersionTooHighError=此程序无法在 %1 版本 %2 或更高版本上安装。
AdminPrivilegesRequired=安装此程序需要管理员权限。
PowerUserPrivilegesRequired=安装此程序需要以管理员或 Power Users 组成员身份登录。
SetupAppRunningError=安装程序检测到 %1 正在运行。%n%n请关闭它，然后按"确定"继续，或按"取消"退出。
UninstallAppRunningError=卸载程序检测到 %1 正在运行。%n%n请关闭它，然后按"确定"继续，或按"取消"退出。

; *** Startup questions
PrivilegesRequiredOverrideTitle=选择安装模式
PrivilegesRequiredOverrideInstruction=选择安装模式
PrivilegesRequiredOverrideText1=%1 可以为所有用户安装（需要管理员权限），或仅为您自己安装。
PrivilegesRequiredOverrideText2=%1 可以仅为您自己安装，或为所有用户安装（需要管理员权限）。
PrivilegesRequiredOverrideAllUsers=为所有用户安装(&A)
PrivilegesRequiredOverrideAllUsersRecommended=为所有用户安装(&A)（推荐）
PrivilegesRequiredOverrideCurrentUser=仅为我安装(&M)
PrivilegesRequiredOverrideCurrentUserRecommended=仅为我安装(&M)（推荐）

; *** Misc. errors
ErrorCreatingDir=无法创建目录 "%1"
ErrorTooManyFilesInDir=无法在目录 "%1" 中创建文件，因为该目录中包含过多文件

; *** Setup common messages
ExitSetupTitle=退出安装
ExitSetupMessage=安装程序尚未完成。如果您现在退出，程序将不会被安装。%n%n是否退出？
AboutSetupMenuItem=关于(&A)...
AboutSetupTitle=关于
AboutSetupMessage=%1 版本 %2%n%3%n%n%1 网站:%n%4
AboutSetupNote=
TranslatorNote=简体中文翻译

; *** Buttons
ButtonBack=< 上一步(&B)
ButtonNext=下一步(&N) >
ButtonInstall=安装(&I)
ButtonOK=确定
ButtonCancel=取消
ButtonYes=是(&Y)
ButtonYesToAll=全部选是(&A)
ButtonNo=否(&N)
ButtonNoToAll=全部选否(&O)
ButtonFinish=完成(&F)
ButtonBrowse=浏览(&B)...
ButtonWizardBrowse=浏览(&B)...
ButtonNewFolder=新建文件夹(&N)

; *** "Select Language" dialog messages
SelectLanguageTitle=选择安装语言
SelectLanguageLabel=选择安装过程中使用的语言:

; *** Common wizard text
ClickNext=按"下一步"继续，或按"取消"退出安装程序。
BeveledLabel=
BrowseDialogTitle=选择目录
BrowseDialogLabel=从列表中选择一个目录，然后按"确定"。
NewFolderName=新建文件夹

; *** "Welcome" wizard page
WelcomeLabel1=欢迎使用 [name] 安装向导
WelcomeLabel2=本程序将在您的计算机上安装 [name/ver]。%n%n建议您在继续之前关闭所有正在运行的其他应用程序。

; *** "Password" wizard page
WizardPassword=密码
PasswordLabel1=此安装程序受密码保护。
PasswordLabel3=请输入密码，然后按"下一步"。
PasswordEditLabel=密码(&P):
IncorrectPassword=密码不正确，请重试。

; *** "License Agreement" wizard page
WizardLicense=许可协议
LicenseLabel=请在继续之前阅读以下重要信息。
LicenseLabel3=请阅读以下许可协议。您必须在继续安装之前接受此协议的条款。
LicenseAccepted=我接受许可协议(&A)
LicenseNotAccepted=我不接受许可协议(&D)

; *** "Information" wizard pages
WizardInfoBefore=信息
InfoBeforeLabel=请在继续之前阅读以下重要信息。
InfoBeforeClickLabel=当您准备继续时，请按"下一步"。
WizardInfoAfter=信息
InfoAfterLabel=请在继续之前阅读以下重要信息。
InfoAfterClickLabel=当您准备继续时，请按"下一步"。

; *** "User Information" wizard page
WizardUserInfo=用户信息
UserInfoDesc=请输入您的信息
UserInfoName=用户名(&U):
UserInfoOrg=单位(&O):
UserInfoSerial=序列号(&S):
UserInfoNameRequired=请输入用户名。

; *** "Select Destination Location" wizard page
WizardSelectDir=选择安装位置
SelectDirDesc=应将 [name] 安装到何处？
SelectDirLabel3=安装程序将把 [name] 安装到以下目录中。
SelectDirBrowseLabel=按"下一步"继续。如果您想选择其他目录，请按"浏览"。
DiskSpaceGBLabel=至少需要 [gb] GB 可用磁盘空间。
DiskSpaceMBLabel=至少需要 [mb] MB 可用磁盘空间。
CannotInstallToNetworkDrive=无法安装到网络驱动器。
CannotInstallToUNCPath=无法安装到 UNC 路径。
InvalidPath=您必须输入带有驱动器号的完整路径，例如:%n%nC:\APP%n%n或 UNC 路径，格式为:%n%n\\服务器\共享名
InvalidDrive=您选择的驱动器或 UNC 共享不存在或不可访问。请选择其他路径。
DiskSpaceWarningTitle=磁盘空间不足
DiskSpaceWarning=安装程序至少需要 %1 KB 的可用空间，但所选驱动器只有 %2 KB 可用。%n%n是否仍要继续？
DirNameTooLong=目录名或路径过长。
InvalidDirName=目录名无效。
BadDirName32=目录名不能包含以下任何字符:%n%n%1
DirExistsTitle=目录已存在
DirExists=目录%n%n%1%n%n已存在。是否仍要安装到此目录？
DirDoesntExistTitle=目录不存在
DirDoesntExist=目录%n%n%1%n%n不存在。是否要创建它？

; *** "Select Components" wizard page
WizardSelectComponents=选择组件
SelectComponentsDesc=应安装哪些组件？
SelectComponentsLabel2=选中您要安装的组件，取消选中您不想安装的组件。按"下一步"继续。
FullInstallation=完整安装
; if possible don't translate 'Compact' as 'Minimal' (I mean 'Minimal' in your language)
CompactInstallation=紧凑安装
CustomInstallation=自定义安装
NoUninstallWarningTitle=组件已存在
NoUninstallWarning=安装程序检测到以下组件已在您的计算机上安装:%n%n%1%n%n取消选中这些组件将不会卸载它们。%n%n是否继续？
ComponentSize1=%1 KB
ComponentSize2=%1 MB
ComponentsDiskSpaceGBLabel=当前选择至少需要 [gb] GB 磁盘空间。
ComponentsDiskSpaceMBLabel=当前选择至少需要 [mb] MB 磁盘空间。

; *** "Select Additional Tasks" wizard page
WizardSelectTasks=选择附加任务
SelectTasksDesc=应执行哪些附加任务？
SelectTasksLabel2=选择在安装 [name] 期间应执行的附加任务，然后按"下一步"。

; *** "Select Start Menu Folder" wizard page
WizardSelectProgramGroup=选择开始菜单文件夹
SelectStartMenuFolderDesc=应在何处放置快捷方式？
SelectStartMenuFolderLabel3=安装程序将在以下开始菜单文件夹中创建快捷方式。
SelectStartMenuFolderBrowseLabel=按"下一步"继续。如果您想选择其他文件夹，请按"浏览"。
MustEnterGroupName=请输入文件夹名称。
GroupNameTooLong=文件夹名称或路径过长。
InvalidGroupName=文件夹名称无效。
BadGroupName=文件夹名称不能包含以下任何字符:%n%n%1
NoProgramGroupCheck2=不创建开始菜单文件夹(&D)

; *** "Ready to Install" wizard page
WizardReady=准备安装
ReadyLabel1=安装程序现在准备开始安装 [name]。
ReadyLabel2a=按"安装"继续，或按"上一步"查看或更改设置。
ReadyLabel2b=按"安装"继续。
ReadyMemoUserInfo=用户信息:
ReadyMemoDir=安装目录:
ReadyMemoType=安装类型:
ReadyMemoComponents=选定的组件:
ReadyMemoGroup=开始菜单文件夹:
ReadyMemoTasks=附加任务:

; *** TDownloadWizardPage wizard page and DownloadTemporaryFile
DownloadingLabel2=正在下载文件...
ButtonStopDownload=停止下载(&S)
StopDownload=确定要停止下载吗？
ErrorDownloadAborted=下载已中止
ErrorDownloadFailed=下载失败: %1 %2
ErrorDownloadSizeFailed=获取文件大小失败: %1 %2
ErrorProgress=无效的进度: %1 / %2
ErrorFileSize=无效的文件大小: 预期 %1，实际 %2

; *** TExtractionWizardPage wizard page and ExtractArchive
ExtractingLabel=正在解压文件...
ButtonStopExtraction=停止解压(&S)
StopExtraction=确定要停止解压吗？
ErrorExtractionAborted=解压已中止
ErrorExtractionFailed=解压失败: %1

; *** Archive extraction failure details
ArchiveIncorrectPassword=密码不正确
ArchiveIsCorrupted=存档文件已损坏
ArchiveUnsupportedFormat=不支持的存档格式

; *** "Preparing to Install" wizard page
WizardPreparing=正在准备安装
PreparingDesc=安装程序正在准备将 [name] 安装到您的计算机上。
PreviousInstallNotCompleted=之前程序的安装/卸载未完成。您需要重新启动计算机以完成安装。%n%n重新启动后，请再次运行安装程序以完成 [name] 的安装。
CannotContinue=无法继续。按"取消"退出。
ApplicationsFound=以下应用程序正在使用需要由安装程序更新的文件。建议您允许安装程序自动关闭这些应用程序。
ApplicationsFound2=以下应用程序正在使用需要由安装程序更新的文件。建议您允许安装程序自动关闭这些应用程序。安装完成后，安装程序将尝试重新启动这些应用程序。
CloseApplications=自动关闭应用程序(&A)
DontCloseApplications=不关闭应用程序(&D)
ErrorCloseApplications=安装程序无法自动关闭所有应用程序。建议您手动关闭所有使用需要由安装程序更新的文件的应用程序。
PrepareToInstallNeedsRestart=安装程序需要重新启动您的计算机，然后重新运行以完成 [name] 的安装。%n%n是否立即重新启动？

; *** "Installing" wizard page
WizardInstalling=正在安装
InstallingLabel=请稍候，安装程序正在将 [name] 安装到您的计算机上。

; *** "Setup Completed" wizard page
FinishedHeadingLabel=[name] 安装完成
FinishedLabelNoIcons=[name] 已成功安装到您的计算机上。
FinishedLabel=[name] 已成功安装到您的计算机上。
ClickFinish=按"完成"退出安装程序。
FinishedRestartLabel=要完成 [name] 的安装，必须重新启动计算机。是否立即重新启动？
FinishedRestartMessage=要完成 [name] 的安装，必须重新启动计算机。%n%n是否立即重新启动？
ShowReadmeCheck=是，我想查看自述文件。
YesRadio=是，立即重新启动(&Y)
NoRadio=否，稍后重新启动(&N)
; used for example as 'Run MyProg.exe'
RunEntryExec=运行 %1
; used for example as 'View Readme.txt'
RunEntryShellExec=查看 %1

; *** "Setup Needs the Next Disk" stuff
ChangeDiskTitle=安装程序需要下一张磁盘
SelectDiskLabel2=请插入磁盘 %1，然后按"确定"。%n%n如果文件位于其他位置，请输入正确的路径或按"浏览"。
PathLabel=路径(&P):
FileNotInDir2=在 "%2" 中找不到文件 "%1"。请插入正确的磁盘或选择其他目录。
SelectDirectoryLabel=请指定下一张磁盘的位置。

; *** Installation phase messages
SetupAborted=安装程序未完成。%n%n请修正问题，然后重新运行安装程序。
AbortRetryIgnoreSelectAction=选择操作
AbortRetryIgnoreRetry=重试(&R)
AbortRetryIgnoreIgnore=忽略错误并继续(&I)
AbortRetryIgnoreCancel=取消安装
RetryCancelSelectAction=选择操作
RetryCancelRetry=重试(&R)
RetryCancelCancel=取消

; *** Installation status messages
StatusClosingApplications=正在关闭应用程序...
StatusCreateDirs=正在创建目录...
StatusExtractFiles=正在解压文件...
StatusDownloadFiles=正在下载文件...
StatusCreateIcons=正在创建快捷方式...
StatusCreateIniEntries=正在创建 INI 条目...
StatusCreateRegistryEntries=正在创建注册表条目...
StatusRegisterFiles=正在注册文件...
StatusSavingUninstall=正在保存卸载信息...
StatusRunProgram=正在完成安装...
StatusRestartingApplications=正在重新启动应用程序...
StatusRollback=正在回滚更改...

; *** Misc. errors
ErrorInternal2=内部错误: %1
ErrorFunctionFailedNoCode=%1 失败
ErrorFunctionFailed=%1 失败，代码: %2
ErrorFunctionFailedWithMessage=%1 失败，代码: %2.%n%3
ErrorExecutingProgram=无法执行文件:%n%1

; *** Registry errors
ErrorRegOpenKey=打开注册表项时出错:%n%1\%2
ErrorRegCreateKey=创建注册表项时出错:%n%1\%2
ErrorRegWriteKey=写入注册表项时出错:%n%1\%2

; *** INI errors
ErrorIniEntry=写入 INI 文件 "%1" 时出错。

; *** File copying errors
FileAbortRetryIgnoreSkipNotRecommended=跳过此文件（不推荐）
FileAbortRetryIgnoreIgnoreNotRecommended=忽略错误并继续（不推荐）
SourceIsCorrupted=源文件已损坏。
SourceDoesntExist=源文件 "%1" 不存在。
SourceVerificationFailed=源文件验证失败: %1
VerificationSignatureDoesntExist=签名文件 "%1" 不存在
VerificationSignatureInvalid=签名文件 "%1" 无效
VerificationKeyNotFound=签名文件 "%1" 使用了未知密钥
VerificationFileNameIncorrect=文件名不正确
VerificationFileTagIncorrect=文件标记不正确
VerificationFileSizeIncorrect=文件大小不正确
VerificationFileHashIncorrect=文件哈希值不正确
ExistingFileReadOnly2=现有文件无法替换，因为它被标记为只读。
ExistingFileReadOnlyRetry=移除只读属性并重试(&R)
ExistingFileReadOnlyKeepExisting=保留现有文件(&K)
ErrorReadingExistingDest=尝试读取现有文件时出错:
FileExistsSelectAction=选择操作
FileExists2=文件已存在
FileExistsOverwriteExisting=覆盖现有文件(&O)
FileExistsKeepExisting=保留现有文件(&K)
FileExistsOverwriteOrKeepAll=对后续所有冲突执行此操作(&D)
ExistingFileNewerSelectAction=选择操作
ExistingFileNewer2=现有文件比安装程序尝试安装的文件更新。
ExistingFileNewerOverwriteExisting=覆盖现有文件(&O)
ExistingFileNewerKeepExisting=保留现有文件（推荐）(&K)
ExistingFileNewerOverwriteOrKeepAll=对后续所有冲突执行此操作(&D)
ErrorChangingAttr=尝试更改现有文件的属性时出错:
ErrorCreatingTemp=尝试在目标目录中创建文件时出错:
ErrorReadingSource=尝试读取源文件时出错:
ErrorCopying=尝试复制文件时出错:
ErrorDownloading=尝试下载文件时出错:
ErrorExtracting=尝试解压文件时出错:
ErrorReplacingExistingFile=尝试替换现有文件时出错:
ErrorRestartReplace=RestartReplace 失败:
ErrorRenamingTemp=尝试重命名目标目录中的文件时出错:
ErrorRegisterServer=无法注册 DLL/OCX: %1
ErrorRegSvr32Failed=RegSvr32 失败，退出代码: %1
ErrorRegisterTypeLib=无法注册类型库: %1

; *** Uninstall display name markings
; used for example as 'My Program (32-bit)'
UninstallDisplayNameMark=%1 (%2)
; used for example as 'My Program (32-bit, All users)'
UninstallDisplayNameMarks=%1 (%2, %3)
UninstallDisplayNameMark32Bit=32 位
UninstallDisplayNameMark64Bit=64 位
UninstallDisplayNameMarkAllUsers=所有用户
UninstallDisplayNameMarkCurrentUser=当前用户

; *** Post-installation errors
ErrorOpeningReadme=尝试打开自述文件时出错。
ErrorRestartingComputer=安装程序无法重新启动计算机。请手动重新启动。

; *** Uninstaller messages
UninstallNotFound="%1" 文件不存在。无法卸载。
UninstallOpenError="%1" 文件无法打开。无法卸载。
UninstallUnsupportedVer="%1" 卸载日志文件的格式不被此版本的卸载程序识别。无法卸载。
UninstallUnknownEntry=卸载日志中发现未知条目 (%1)
ConfirmUninstall=确定要完全卸载 %1 及其所有组件吗？
UninstallOnlyOnWin64=此程序只能在 64 位 Windows 上卸载。
OnlyAdminCanUninstall=此程序只能由具有管理员权限的用户卸载。
UninstallStatusLabel=请稍候，正在从您的计算机中卸载 %1。
UninstalledAll=%1 已成功从您的计算机中卸载。
UninstalledMost=%1 已从您的计算机中卸载。%n%n某些组件无法移除。您可以手动移除它们。
UninstalledAndNeedsRestart=要完成 %1 的卸载，必须重新启动计算机。%n%n是否立即重新启动？
UninstallDataCorrupted="%1" 文件已损坏。无法卸载。

; *** Uninstallation phase messages
ConfirmDeleteSharedFileTitle=删除共享文件？
ConfirmDeleteSharedFile2=系统提示以下共享文件不再被任何程序使用。是否卸载此共享文件？%n%n如果不确定，请按"否"。
SharedFileNameLabel=文件名:
SharedFileLocationLabel=位置:
WizardUninstalling=卸载状态
StatusUninstalling=正在卸载 %1...

; *** Shutdown block reasons
ShutdownBlockReasonInstallingApp=正在安装 %1。
ShutdownBlockReasonUninstallingApp=正在卸载 %1。

; The custom messages below aren't used by Setup itself, but if you make
; use of them in your scripts, you'll want to translate them.

[CustomMessages]

NameAndVersion=%1 版本 %2
AdditionalIcons=附加图标
CreateDesktopIcon=创建桌面快捷方式(&D)
CreateQuickLaunchIcon=在快速启动栏创建快捷方式(&Q)
ProgramOnTheWeb=%1 网站
UninstallProgram=卸载 %1
LaunchProgram=启动 %1
AssocFileExtension=将 %1 与 %2 文件扩展名关联(&A)
AssocingFileExtension=正在将 %1 与 %2 文件扩展名关联...
AutoStartProgramGroupDescription=自动启动:
AutoStartProgram=开机自动启动 %1
AddonHostProgramNotFound=%1 无法安装到您选择的目录中。%n%n是否继续？
