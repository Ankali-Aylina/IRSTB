# TemperatureControlV3 — AI 编码代理指南

## 构建与运行

- **项目格式**：VS2022 解决方案（`TemperatureControlV3.sln`），单个 vcxproj
- **构建配置**：仅 `Debug|x64` 和 `Release|x64`（纯 64 位）
- **工具链**：v145（VS 2022），C++20（`stdcpp20`），Unicode，Windows 子系统
- **Qt**：6.11.1 msvc2022_64，模块：`core;gui;widgets;concurrent`（通过 Qt VS Tools 集成）
- **Windows SDK**：10.0.28000.0
- **NuGet**：C++/WinRT 2.0.240405.15（仅构建工具，项目中没有 `.idl` 文件，无 WinRT API）
- **构建后步骤**：将 `res\lib\inteltemp.dll`、`IntelMSR.bin`、`PawnIO_setup.exe` 复制到 `$(OutDir)`
- **运行**：需要 `config.ini` 和 `res/` 目录在 exe 旁边；发布版本需要 `platforms/`、`styles/` 等 Qt 插件 DLL

## 架构

详见 `/memories/repo/architecture.md`。简要说明：

```
main.cpp → ApplicationBootstrap::run()
  ├── UAC 提权（runas）
  ├── 单实例锁（QLockFile）
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

`IniManagement`（`IniManagement.h`）通过 `IConfigProvider` 接口包装 `QSettings`（INI 格式）。通过 `AppModuleManager` 注入到模块中。**切勿**直接使用 `IniManagement::instance()` 访问配置——使用注入的 `m_config` 指针。

### 日志记录

`LogManagement`（`LogManagement.h`）是全局单例。模块发出 `logMessage(level, text)` 信号；TCV3 将它们连接到 `LogManagement::instance()`。**绝不**创建新的 `LogManagement` 实例。

### DLL 加载

所有原生 DLL 通过 `NativeLibraryLoader`（`NativeLibraryLoader.h`）加载——一次性加载，使用初始化列表批量解析符号。**切勿**直接使用原始 `QLibrary`。

### 线程与原子操作

- TCCore 和 BLEThread 之间的通信通过信号/槽，使用 `Qt::QueuedConnection` 连接
- 使用 `std::atomic<bool>` 配合 `memory_order_acquire/release`（非 `seq_cst`）作为停止标志
- `BLEThread` 使用 `static std::atomic<BLEThread*> s_activeScanner` 将无上下文的 DLL 回调路由到正确的实例——添加新的 BLE 回调时保留此模式

## 约定

- **命名**：类使用 PascalCase，成员使用 `m_` 前缀，静态常量使用 `k` 前缀（`kMinDelay`）
- **智能指针**：所有权使用 `std::unique_ptr`，Qt 作用域使用 `QScopedPointer`
- **信号/槽**：使用现代函数指针语法（`connect(sender, &Sender::sig, receiver, &Receiver::slot)`），绝不使用基于字符串的连接
- **注释**：公开声明使用 C# 风格 XML 文档注释（`/// <summary>`、`/// <param>`）
- **语言**：UI 字符串、日志和注释均使用中文；标识符使用英文
- **`cpp.hint`**：存在以支持 Qt 宏的 IntelliSense（`#define slots`、`#define Q_OBJECT`）

## 常见陷阱

1. **不要绕过 `IAppModule` 接口**——始终通过 `AppModuleManager` 管理子系统生命周期
2. **不要使用 `IniManagement::instance()` 进行配置访问**——使用注入的 `IConfigProvider*`
3. **不要创建新的 `LogManagement` 实例**——使用 `LogManagement::instance()`
4. **不要引入原始 `QLibrary` 使用**——使用 `NativeLibraryLoader`
5. **不要使用 `memory_order_seq_cst`**——为停止标志使用 acquire/release
6. **不要添加全局/静态状态**——项目已重构以消除这些状态；改为使用实例成员和依赖注入
7. **跨线程信号连接时**——确保使用 `Qt::QueuedConnection`
8. **添加新的源文件时**——更新 `TemperatureControlV3.vcxproj` 和 `.vcxproj.filters`
