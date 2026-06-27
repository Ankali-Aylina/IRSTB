# TemperatureControlV3 — AI 编码代理指南

## 构建与运行

- **项目格式**：VS2022 解决方案（`TemperatureControlV3.sln`），单个 vcxproj
- **构建配置**：仅 `Debug|x64` 和 `Release|x64`（纯 64 位）
- **工具链**：v145（VS 2022），C++20（`stdcpp20`），Unicode，Windows 子系统
- **Qt**：6.11.1 msvc2022_64，模块：`core;gui;widgets;concurrent`（通过 Qt VS Tools 集成）
- **Windows SDK**：10.0.28000.0
- **运行**：Release 构建下只需 `config.ini`（首次运行自动生成）和 Qt 运行时 DLL（`platforms/`、`styles/` 等）；原生 DLL 和 `bin/` 已嵌入 exe，启动时自动提取到 `%TEMP%`

## 架构

详见 `/memories/repo/architecture.md`。简要说明：

```
main.cpp → ApplicationBootstrap::run()
  ├── UAC 提权（runas）
  ├── 单实例锁（QLockFile）
  ├── ResourceExtractor::extract()（从 QRC 提取 DLL 等到 %TEMP%）
  ├── PawnIO 驱动版本检查
  └── TCV3 主窗口（QMainWindow）
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

所有原生 DLL 通过 `NativeLibraryLoader`（`NativeLibraryLoader.h`）从**文件系统**加载（`QLibrary::load("xxx.dll")`）。但 DLL 文件不再需要单独分发——它们已嵌入 `TCV3.qrc`，启动时由 `ResourceExtractor` 自动提取到 `%TEMP%/TemperatureControlV3_Resources/`：

| DLL 文件            | 加载位置  | 用途                            |
| ------------------- | --------- | ------------------------------- |
| `Cpu_Dll.dll`       | TCCore    | CPU 类型检测                    |
| `inteltemp.dll`     | TCCore    | Intel CPU 温度                  |
| `amd_dll.dll`       | TCCore    | AMD CPU 温度                    |
| `nv_dll.dll`        | TCCore    | NVIDIA GPU 温度                 |
| `WinRT_BLE_DLL.dll` | BLEThread | 蓝牙 LE 通信（WinRT/C++/WinRT） |

> **工作原理**：`ApplicationBootstrap::run()` 调用 `ResourceExtractor::extract()` → 从 QRC 复制文件到 `%TEMP%` → 调用 `SetDllDirectoryW` 将临时目录加入 DLL 搜索路径 → `NativeLibraryLoader` 从搜索路径中找到 DLL。版本标记文件确保只在版本变化时重新提取。

加载时使用初始化列表批量解析符号，**切勿**直接使用原始 `QLibrary`。

**QRC 资源路径前缀**：所有 QRC 路径使用 `:/TCV3/` 前缀（如 `:/TCV3/res/lib/Cpu_Dll.dll`），不要使用 `:/res/...`。

**添加新的运行时文件时**：将文件放入 `res/lib/`（或 `res/lib/bin/`），在 `TCV3.qrc` 中注册，并在 `ResourceExtractor.cpp` 的 `kEntries[]` 中添加映射。

### 信号/槽中枢架构

**TCV3 是唯一的信号/槽中枢（Hub）**。模块之间绝不直接连接——所有跨模块连接均在 `TCV3::TCV3()` 构造函数中完成：

```
TCCore::controlDataUpdated  ──→  BLEThread::controlFan       (QueuedConnection)
TCCore::updateConnectionStatus ──→ BLEThread::updateConnectionStatus
TCV3::setFan*Mode             ──→  BLEThread::*Mode
BLEThread::ble*               ──→  BluetoothStatusPresenter::on*
```

**动态连接管理**：`m_fanControlConnection` 在自动/手动模式间切换：

- 自动模式：连接 `controlDataUpdated → controlFan`（温度驱动调速）
- 静音/性能模式：断开连接（仅手动控制）
- `setFanMode()` 同时管理按钮 UI 状态和此连接

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
| `"BLE"` | BLEThread | `Init`, `TargetName`, `TargetServiceUUID`, `TargetCharacteristicUUID`, `TargetID`                                      |
| `"UI"`  | TCV3      | `dataTxDelay`                                                                                                          |

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
| `Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Widgets.dll`, `Qt6Network.dll`, `Qt6Svg.dll` | Qt 运行时                                            |
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
