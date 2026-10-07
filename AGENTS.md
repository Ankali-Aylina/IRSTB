# TemperatureControlV3 — AI 编码代理指南

## 构建与运行

- **项目格式**：VS2022 解决方案（`TemperatureControlV3.sln`），单个 vcxproj
- **构建配置**：仅 `Debug|x64` 和 `Release|x64`（纯 64 位）
- **工具链**：v145（VS 2022），C++20（`stdcpp20`），Unicode，Windows 子系统
- **Qt**：6.11.1 msvc2022_64，模块：`core;gui;widgets;concurrent`（通过 Qt VS Tools 集成）
- **Windows SDK**：10.0.28000.0
- **运行**：Release 构建下只需 `config.ini`（首次运行自动生成）和 Qt 运行时 DLL（`platforms/`、`styles/` 等）；原生 DLL 和 `bin/` 已嵌入 exe，启动时自动提取到 `%LOCALAPPDATA%/TemperatureControlV3/TemperatureControlV3_Resources/`

## 版本号规范（改动代码前先读）

### 编号规则

格式固定为 `主.次.修订.构建` 四段，**本项目约定第 4 段恒为 0**（历史版本也遵循此约定：4.0.0.0 / 3.4.1.0 / 3.4.0.1）：

| 段位 | 何时递增 | 典型场景 |
| --- | --- | --- |
| **主版本** | 不兼容的大改造 | 框架更换（Widgets → QML 那次是 4.0.0.0）、通信协议不兼容变更 |
| **次版本** | 新增用户可见功能 | 新增关机散热、新增系统通知、新增设置项；**下位机固件协议变更也算**（用户必须同步刷固件） |
| **修订号** | 仅修 bug，无新功能 | 修复死锁、修复解析错误、修复缓存判据 |
| 构建号 | 恒为 0 | — |

判断要点：

- **同一版本号只能对应一份二进制**。只要发布过（`installer/` 里有对应的 zip/Setup），就绝不能再出第二个同号但内容不同的包 —— 用户无法分辨自己装的是哪个。曾踩过：4.2.0.0 已发布后继续改代码仍用同号，导致"内嵌 DLL 换了但提取缓存不更新"的问题难以定位。
- **只要改动会影响用户可见行为，就必须升版本号**，哪怕只是替换一个内嵌 DLL —— `ResourceExtractor` 以版本号为缓存判据（见下），不升号可能导致新资源不生效。
- 未正式发布的中间版本可以复用/作废，但要在 `updatalog.md` 里明确标注"内部版本，未正式发布"。

### 版本号的位置（两处，必须同步改）

| 文件 | 内容 | 影响范围 |
| --- | --- | --- |
| `resource.h` | `APP_VERSION_STR "x.y.z.0"` + `APP_VERSION_COMMA x,y,z,0` | **单一来源**：exe 版本资源（经 `.rc`）、程序内「关于」页显示、`package.ps1` 生成的 zip 名 |
| `installer.iss` | `#define MyAppVersion "x.y.z.0"` | Inno Setup 安装包名与注册表 `AppVersion`（**独立副本，不会自动跟随**） |

### 升版本的完整清单

1. 改 `resource.h` 的两行（字符串 + 逗号形式）
2. 改 `installer.iss` 的 `MyAppVersion`
3. 在 `res/updatalog.md` **顶部**新增 `## vX.Y.Z.0` 条目（程序内「更新日志」页直接读它）
4. 更新 `README.md` 的「版本历史」表
5. **重新构建 Release** —— 版本资源与 QML 内嵌资源（更新日志）都只在构建时才嵌入 exe
6. 确认 `installer/` 里的旧包已挪走或删除，避免误发

### 与资源提取的联动（重要）

`ResourceExtractor` 用 `[App 版本号]` 作缓存判据，并按文件名+大小记录内容清单（`.manifest`）：

- **版本号变化** → 整个提取目录清空重建，`%LOCALAPPDATA%` 下的旧文件不会残留
- **版本号不变但文件内容变了** → 靠 `.manifest` 的大小比对发现并重新提取（清单缺失也视为需要重提取）
- 因此**开发期替换了 `res/lib/` 下的 DLL 后，务必确认应用重新提取**：日志里查 `ResourceExtractor: changed file ...`，或直接核对 `%LOCALAPPDATA%/TemperatureControlV3/TemperatureControlV3_Resources/` 下文件的大小/MD5
- ⚠️ 不要试图用"QRC 内文件大小"比对来判断是否需要重新提取：QRC 是压缩存储，`QFileInfo(":/...").size()` 拿到的是压缩后大小，与磁盘上的解压文件永远不相等（这个判据曾经失效，导致换了 DLL 却不更新）

## 架构

详见 `/memories/repo/architecture.md`。简要说明：

```
main.cpp → ApplicationBootstrap::run()
  ├── UAC 提权（runas）
  ├── 单实例锁（QLockFile）
  ├── ResourceExtractor::extract()（从 QRC 提取 DLL 等到 %LOCALAPPDATA%，并做 Authenticode 校验）
  ├── PawnIO 驱动版本检查
  └── QQmlApplicationEngine 加载 qml/main.qml
        └── QmlBridge（C++↔QML 桥：模块生命周期/托盘/UI 意图转发）
              └── AppModuleManager（拥有 IAppModule 实例）
              ├── TCCore（温度采集 + 风扇控制逻辑）
              └── BLEThread（蓝牙 LE 通信）
```

两个模块在 `start()` 中均创建私有 `QThread`，在其中 `moveToThread` 并运行事件循环。

## 关键模式

### 模块系统（2026-06-20 重构）

所有子系统均实现 `IAppModule`（`IAppModule.h`）：

- `initialize()` — 加载配置/资源
- `start()` — 创建工作线程，开始处理
- `stop(timeoutMs)` — 优雅关闭：设置原子停止标志，`quit()+wait()`，移回主线程

使用 `AppModuleManager`：

```cpp
m_moduleManager->registerModule<TCCore>(m_config);  // 转发构造函数参数
m_moduleManager->registerModule<BLEThread>(m_config);
m_moduleManager->initializeAll();
m_moduleManager->startAll();
// ... 稍后：
m_moduleManager->stopAll();  // 逆序停止（BLE 在 TCCore 之前）
```

### 配置（IConfigProvider）

`IniManagement`（`IniManagement.h`）通过 `IConfigProvider` 接口包装 `QSettings`（INI 格式）。通过 `AppModuleManager` 注入到模块中。使用注入的 `m_config` 指针访问配置。

**首次运行检测**：`IConfigProvider` 提供 `isFirstRun()` / `markFirstRunDone()`，基于 `[App]/FirstRun` 标记（向后兼容旧版 `InitStatus`）。新增配置段时，在对应的 `initConfigFile()` 中使用 `if (!m_config->isFirstRun()) return;` 守卫。

`TemperatureConfig.h` 定义温度控制参数结构体，含 `FanMode` 枚举（`enum class : uint8_t`：Auto=0, Silent=5, Performance=6）和 `fanModeValue()` 转换函数。`loadFromConfig()` 使用本地 lambda `readInt` 减少重复代码。⚠️ `weights` 向量虽为成员，但实际不可从配置自定义——如果 `historySize` 变化，权重会被重置为均匀值 `1.0f`。

### 日志记录

`LogManagement`（`LogManagement.h`）是全局单例。模块发出 `logMessage(level, text)` 信号；TCV3 将它们连接到 `LogManagement::instance()`。**绝不**创建新的 `LogManagement` 实例。

### DLL 加载与资源提取

所有原生 DLL 通过 `NativeLibraryLoader`（`NativeLibraryLoader.h`）从**文件系统**加载（`QLibrary::load("xxx.dll")`）。但 DLL 文件不再需要单独分发——它们已嵌入 `TCV3.qrc`，启动时由 `ResourceExtractor` 自动提取到 `%LOCALAPPDATA%/TemperatureControlV3/TemperatureControlV3_Resources/`（每用户目录 + Authenticode 校验，防 DLL 劫持）：

| DLL 文件            | 加载位置  | 用途                            |
| ------------------- | --------- | ------------------------------- |
| `Cpu_Dll.dll`       | TCCore    | CPU 类型检测                    |
| `inteltemp.dll`     | TCCore    | Intel CPU 温度                  |
| `amd_dll.dll`       | TCCore    | AMD CPU 温度                    |
| `nv_dll.dll`        | TCCore    | NVIDIA GPU 温度                 |
| `WinRT_BLE_DLL.dll` | BLEThread | 蓝牙 LE 通信（WinRT/C++/WinRT） |

> **工作原理**：`ApplicationBootstrap::run()` 调用 `ResourceExtractor::extract()` → 从 QRC 复制文件到 `%LOCALAPPDATA%/TemperatureControlV3/TemperatureControlV3_Resources/` → 对 .dll/.exe/.sys 做 Authenticode 校验（签名无效的文件删除并中止启动）→ 调用 `SetDllDirectoryW` 将该目录加入 DLL 搜索路径 → `NativeLibraryLoader` 从搜索路径中找到 DLL。
>
> **是否需要重新提取的判据**（详见「版本号规范」节）：版本号变化 → 整体清空重建；版本号不变 → 比对 `.manifest` 内容清单（文件名+大小），不一致即重新提取，清单缺失也视为需要重提取。

加载时使用初始化列表批量解析符号，**切勿**直接使用原始 `QLibrary`。

**QRC 资源路径前缀**：所有 QRC 路径使用 `:/TCV3/` 前缀（如 `:/TCV3/res/lib/Cpu_Dll.dll`），不要使用 `:/res/...`。

**添加新的运行时文件时**：将文件放入 `res/lib/`（或 `res/lib/bin/`），在 `TCV3.qrc` 中注册，并在 `ResourceExtractor.cpp` 的 `kEntries[]` 中添加映射。

### 信号/槽中枢架构

**QmlBridge 是唯一的信号/槽中枢（Hub）**（QML 版；原 Widgets 版 TCV3 已移除）。模块之间绝不直接连接——所有跨模块连接均在 `QmlBridge::setupModules()` 中完成：

```
TCCore::controlDataUpdated  ──→  BLEThread::controlFan       (QueuedConnection)
TCCore::updateConnectionStatus ──→ BLEThread::updateConnectionStatus
QmlBridge::setFanMode（QML 调用）──→ BLEThread::*Mode
QmlBridge::startShutdownCooling（QML 调用）──→ BLEThread::shutdownCooling
BLEThread::ble*               ──→  QmlBridge::setBleUiState（QML 属性 bleState/bleStatusText）
```

**动态连接管理**：`m_fanControlConnection` 在自动/手动模式间切换：

- 自动模式：连接 `controlDataUpdated → controlFan`（温度驱动调速）
- 静音/性能模式：断开连接（仅手动控制）
- `setFanMode()` 同时管理按钮 UI 状态和此连接

### 关机散热（Shutdown Cooling）

上位机只负责**下发一次指令**，倒计时由下位机 MCU 独立完成（LPTIM 秒级），因此蓝牙断开/程序退出/电脑关机都不会中断散热：

| 层 | 职责 |
| --- | --- |
| QML | `shutdownPanel` 面板：时长 −/+ 与「开始散热」按钮 → `bridge.changeShutdownMinutes()` / `bridge.startShutdownCooling()` |
| QmlBridge | 时长持久化到 `TC/ShutdownMinutes`（1~10，默认 3，越界重置）；`startShutdownCooling()` 转发给 BLEThread |
| BLEThread | `shutdownCooling(minutes)` 组帧 `T<分钟>\n` 并写入特征值；置位 `m_shutdownActive` |

关键约定：

- **协议常量集中在 `BLEThread.h`**（`kShutdownCmdHeader`、`kMinShutdownMinutes`、`kMaxShutdownMinutes`、`kDefaultShutdownMinutes`）；下位机硬上限 60 分钟（2 位十进制），改 `kMaxShutdownMinutes` 无需重刷下位机固件
- **倒计时期间上位机停止下发温度调速帧**：`BLEThread::controlFan()` 在 `m_shutdownActive` 为真时直接返回，避免每秒一次的无效 BLE 写入与日志
- **取消只能靠"重新选择风扇模式"**：`QmlBridge::setFanMode()` → `BLEThread::notifyShutdownCancelled()` 清标志，随后发出的模式指令（`'0'`~`'6'`）让下位机恢复响应
- **无法读取下位机状态** → **已解决**：`WinRT_BLE_DLL.dll`（源码 `D:\code\WinRT_BLE`，CMake 工程）从 v1.0.0 起导出 `BleSubscribeCharacteristic` / `BleUnsubscribeCharacteristic` / `BleReadCharacteristic` / `BleGetVersion`。BLEThread 在连接成功后自动订阅 FFE1，并把下位机回传写进日志；DLL 若缺少这些导出只提示一条告警，不影响风扇控制
- **通知帧不保证"一行一条"**：实测 `OK.60\n` 会被拆成多条 BLE 通知（先 `OK` 后 `.60\n`）。`onDeviceStatusReport()` 必须先把字节追加到 `m_notifyBuffer` 再按 `\n` 切分，逐条处理；订阅/退订时清空缓冲，避免跨连接的半行数据被拼成错误帧
- **掉线提示去重**：`m_deviceInfo.isFind` 在首次扫描成功后就不再复位，而 `controlFan()` 由温度更新周期性触发（默认 5 s），因此掉线期间会反复刷"BLE连接断开"。统一走 `reportDisconnectedOnce()`（`m_disconnectReported` 标志），并在「连接成功」与「写入成功」时复位该标志；新增会发 `disconnected()` 或该日志的路径时请复用它
- **指令被拒后自动重发一次**：下位机回 `TO`/`WAIT` 表示指令未生效，`scheduleShutdownRetry()` 会在 600 ms 后重发（`m_shutdownRetryUsed` 保证同一轮只重试一次）
- **散热结束后自动切回自动模式**：下位机收尾时只关风扇、**不锁定**，因此收到 `FIN` 后 `BLEThread::restoreAutoModeAfterShutdown()` 会立即下发 `'0'`（走 `autoMode()`，与用户点"自动"同一条路径），风扇随后按温度调速。`m_autoRestoreDone` 保证 FIN 与兜底定时器都触发时只恢复一次；切回自动前会先判断连接状态，未连接则提示用户重连
- **中途断连会丢掉 `FIN`**：下位机照常在内部走完倒计时，但上位机收不到结束回传。`scheduleShutdownFinishedCheck(seconds)` 按 `OK.<秒>` 自报值 +5 s 兜底判定并照常走恢复流程，避免恢复永不发生
- **散热结束要提示用户**：结束后风扇会短暂断电再被自动模式接管，期间界面原本毫无变化。`shutdownCoolingFinished` → `QmlBridge::showShutdownNotice(text, autoDismiss)` → 首页提示条。**信息型提示（autoDismiss=true）6 秒后自动消失、且会被切模式清掉；错误型提示（如"蓝牙未连接"）必须用户手动关闭**，否则自动恢复流程会瞬间把它清掉，用户根本看不到
- **指令没发出去不能只写日志**：未连接/正在连接时原本静默 return，界面表现为"点了没反应"。统一 emit `commandNotSent(reason)`，由 QmlBridge 显示成需手动处理的提示条
- **系统通知用 Qt 托盘 `showMessage()`**：`QmlBridge::showSystemNotification()` 经 `QSystemTrayIcon::showMessage()` 弹出（内部走 `Shell_NotifyIcon`），窗口最小化/隐藏到托盘时同样可见。要点：
  - **通知文案要短**：标题 ≤8 字、正文一句话（如「散热已完成 / 风扇已断电，已切回自动模式」）。通知宽度由系统决定，长文本会被折行截断
  - `kNotifyTimeoutMs = 10000` 停留时长；`kNotifyThrottleMs = 3000` 节流（3 秒内重复通知只弹第一条，防用户连点刷屏）
  - 由 `[App]/ToastNotify` 开关控制（默认开启）；开关打开时立刻弹一条示例，并**绕过节流**（把 `m_lastNotifyMs` 置 0）
  - 托盘不可用时只记 WARNING，界面内提示条仍照常工作
  - ⚠️ **不要改用 WinRT `ToastNotificationManager`**：用「exe 全路径当 AUMID」的写法调用会全部成功（`init_apartment/LoadXml/CreateToastNotifier/Show` 均不报错），但**系统对未注册的 AUMID 静默丢弃通知，用户什么都看不到**（已实测踩坑）。要真出原生卡片必须让系统认识该 AUMID：开始菜单放带 AUMID 属性的快捷方式，或走 MSIX 打包 / 注册 COM 通知激活器——对绿色免安装形态代价过高

### 下位机固件版本查询

连接并订阅 FFE1 成功后，`BLEThread::queryDeviceVersion()` 自动发送 `"V\n"`，下位机（固件 **1.3.0+**）回传 `"V<固件版本>.<协议版本>"`（如 `V1.3.0.1.1`）：

| 要点 | 做法 |
| --- | --- |
| 帧解析 | `onDeviceStatusReport()` 里以 `V` 开头的行交给 `handleVersionResponse()` 并 `continue`——版本回传不是"指令执行结果"，不能混进散热事件流 |
| 显示位置 | 上位机版本号下方（「关于」页）。⚠️ **不能放设置页**：设置页没有滚动容器，可用高度仅 468px，已用 424px，再加一个 64px 面板会溢出 |
| 旧固件降级 | 固件 < 1.3.0 不会回传。`kVersionQueryTimeoutMs = 2500` 超时后 emit `firmwareVersionReceived("", "", true)`，界面显示"版本未知"。**旧固件仍支持关机散热（1.2.0 起），因此超时时按"支持"处理**，避免误禁用功能 |
| 最低版本常量 | `kMinFwMajor/MinorForShutdown`（1.2.0）、`kMinFwMajor/MinorForVersionQuery`（1.3.0）；`firmwareSupportsShutdown` 由回传版本比对得出，界面据此标红提示 |
| 版本来源 | 固件侧：`main.h` 的 `FW_VERSION_*` + `fw_version.h` 的 `FW_PROTOCOL_VERSION_STR`；协议版本只在"上位机需同步适配"时递增 |

### 错误处理约定

| 场景         | 模式                                                                               |
| ------------ | ---------------------------------------------------------------------------------- |
| 温度读取失败 | 返回哨兵值 `-99`（错误）、`-100`（不支持的 CPU）                                   |
| 温度有效性   | 仅 `temp > 0` 时发出信号                                                           |
| DLL 加载失败 | `NativeLibraryLoader::load()` 返回 `bool`；emit `logMessage(LOG_ERROR)` + 提前返回 |
| 配置读取     | 使用前检查 `QVariant::isValid()`                                                   |
| BLE 操作     | Mutex 保护的 `isFind`/`isConnected` 检查                                           |
| 配置范围     | 使用 `kMinDelay`/`kMaxDelay` constexpr 边界；越界则重置为默认值                    |

### 配置段命名约定

| Section | 使用方    | 键值                                                                                                                   |
| ------- | --------- | ---------------------------------------------------------------------------------------------------------------------- |
| `"App"` | 全局      | `FirstRun`（首次运行标记，替代旧版各段 `InitStatus`）                                                                  |
| `"TC"`  | TCCore    | `DataTransmissionDelay`, `InitCpuTemp`, `InitGpuTemp`, `CpuStep`, `GpuStep`, `WarningCpu`, `WarningGpu`, `HistorySize` |
| `"TC"`  | QmlBridge | `ShutdownMinutes`（关机散热时长，1~10 分钟，默认 3；在 UI 上点一次 +/- 即写盘）                                        |
| `"App"` | QmlBridge | `ThemeMode`, `BackdropType`, `ToastNotify`（系统通知开关，默认开启）                                                   |
| `"BLE"` | BLEThread | `Init`, `TargetName`, `TargetServiceUUID`, `TargetCharacteristicUUID`, `TargetID`                                      |
| `"UI"`  | TCV3      | （已废弃 `dataTxDelay`，延时统一存 `TC/DataTransmissionDelay`，旧键自动迁移）                                  |

> ⚠️ 开机自启（AutoStart）**不在 config.ini 中**——直接读写注册表 `HKCU\...\Run`。`isAutoStartEnabled()` / `setAutoStart()` 操作注册表，不要写 config。

### 线程与原子操作

- TCCore 和 BLEThread 之间的通信通过信号/槽，使用 `Qt::QueuedConnection` 连接
- 使用 `std::atomic<bool>` 配合 `memory_order_acquire/release`（非 `seq_cst`）作为停止标志
- `BLEThread` 使用 `static std::atomic<BLEThread*> s_activeScanner` 将无上下文的 DLL 回调路由到正确的实例——添加新的 BLE 回调时保留此模式
- ⚠️ `TCCore::controlDataUpdated(char*)` 传递 `static char buffer[8]`——跨线程 `QueuedConnection` 下数据被拷贝，是安全的；但如果改为 `DirectConnection` 会出现悬空指针

## 发布打包

Release 构建下，原生 DLL 和 `bin/` 已全部嵌入 exe 的 QRC 中。发布时只需分发以下文件：

**必须的文件：**

| 文件/目录                                                                     | 说明                                                 |
| ----------------------------------------------------------------------------- | ---------------------------------------------------- |
| `TemperatureControlV3.exe`                                                    | 主程序（已内嵌所有原生 DLL、`bin/`、图标、更新日志） |
| `Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Qml.dll`, `Qt6Quick.dll`, `Qt6QuickControls2.dll`, `Qt6Widgets.dll`, `Qt6Network.dll`, `Qt6Svg.dll` | Qt 运行时
| `qml/` | QML 运行时插件（`windeployqt --qmldir` 自动复制，**必须**，否则 QML 界面无法加载） |                                            |
| `platforms/qwindows.dll`                                                      | Qt 平台插件（**必须**，否则无法创建窗口）            |
| `styles/`                                                                     | Qt 样式插件                                          |
| `imageformats/`, `iconengines/`                                               | 图片格式和 SVG 支持                                  |
| `networkinformation/`, `tls/`                                                 | 网络状态和 SSL                                       |
| `D3Dcompiler_47.dll`, `opengl32sw.dll`                                        | 图形依赖                                             |

**注意**：`config.ini` 首次运行自动生成，无需手动提供。

**可精简的：**

| 文件            | 说明                   |
| --------------- | ---------------------- |
| `translations/` | 如只需中文可删除       |
| `app.log`       | 运行时日志，发布时删除 |

**打包步骤：**

1. 构建 Release 配置
2. 删除输出目录中的 `app.log`
3. 使用 `windeployqt TemperatureControlV3.exe --no-translations` 补全 Qt 依赖
4. 将所有文件打包为 zip
5. 提醒用户需安装 [VC Redist x64](https://aka.ms/vs/17/release/vc_redist.x64.exe)（项目使用 `/MD` 动态链接）

**自动化脚本**：`package.ps1` — 一键编译 + windeployqt + 精简 + 打包。

```powershell
.\package.ps1                           # 完整流程
.\package.ps1 -SkipBuild                # 跳过编译，直接打包
.\package.ps1 -QtPath "D:\Qt\6.11.1\msvc2022_64"  # 指定 Qt 路径
```

**安装包**：`installer.iss` — [Inno Setup](https://jrsoftware.org/isinfo.php) 脚本，生成带桌面快捷方式、卸载支持的安装程序。

1. 先运行 `.\package.ps1 -SkipBuild` 准备 Release 文件
2. 用 Inno Setup 打开 `installer.iss` → 编译 → 输出到 `installer\` 目录
3. 自动检测 VC Redist，缺失时引导用户下载

## 约定

- **命名**：类使用 PascalCase，成员使用 `m_` 前缀，静态常量使用 `k` 前缀（`kMinDelay`）
- **智能指针**：所有权使用 `std::unique_ptr`，Qt 作用域使用 `QScopedPointer`
- **信号/槽**：使用现代函数指针语法（`connect(sender, &Sender::sig, receiver, &Receiver::slot)`），绝不使用基于字符串的连接
- **注释**：公开声明使用 C# 风格 XML 文档注释（`/// <summary>`、`/// <param>`）
- **语言**：UI 字符串、日志和注释均使用中文；标识符使用英文
- **`cpp.hint`**：存在以支持 Qt 宏的 IntelliSense（`#define slots`、`#define Q_OBJECT`）

## 常见陷阱

1. **不要绕过 `IAppModule` 接口**——始终通过 `AppModuleManager` 管理子系统生命周期
2. **不要创建新的 `LogManagement` 实例**——使用 `LogManagement::instance()`
3. **不要引入原始 `QLibrary` 使用**——使用 `NativeLibraryLoader`
4. **不要使用 `memory_order_seq_cst`**——为停止标志使用 acquire/release
5. **不要添加全局/静态状态**——项目已重构以消除这些状态；改为使用实例成员和依赖注入
6. **跨线程信号连接时**——确保使用 `Qt::QueuedConnection`
7. **添加新的源文件时**——更新 `TemperatureControlV3.vcxproj` 和 `.vcxproj.filters`
8. **vcxproj 文件分类**：`QObject` 子类头文件必须标为 `<QtMoc>`（非 `<ClInclude>`），否则 MOC 不生成元对象代码导致链接错误；`.ui` 文件必须标为 `<QtUic>`；`.qrc` 文件标为 `<QtRcc>`
9. **DLL 初始化必须在主线程**：`inteltemp_initialize` 等 DLL init 函数必须在主线程调用（`TCCore::initialize()` 中），不要移到工作线程
10. **`controlDataUpdated(char*)` 传递静态缓冲区指针**——跨线程 `QueuedConnection` 安全（数据拷贝），但绝不能改为 `DirectConnection`
11. **`stop()` 后必须 `moveToThread(QThread::currentThread())`**——在将线程指针置空之前，否则析构时出现 QObject 父子关系问题
12. **`stop()` 后必须 `moveToThread(QThread::currentThread())`**——在将线程指针置空之前，否则析构时出现 QObject 父子关系问题
13. **开机自启只读注册表**：`isAutoStartEnabled()` 读的是注册表而非 config.ini，不要往 config 写 `AutoStart` 键
14. **配置值读取后必须校验范围**：`TemperatureConfig::loadFromConfig()` 和 `TCCore::getDataTrDelay()` 均有范围 clamp；新增配置读取时遵循此模式
