#pragma once

/// <summary>
/// 应用程序模块接口——所有子系统（TCCore、BLEThread 等）实现此接口，
/// 由 AppModuleManager 统一管理生命周期和线程调度。
/// </summary>
class IAppModule
{
public:
    virtual ~IAppModule() = default;

    /// <summary>初始化模块（加载配置、连接内部信号）</summary>
    /// <returns>初始化是否成功</returns>
    virtual bool initialize() = 0;

    /// <summary>启动模块（开始工作线程等）</summary>
    virtual void start() = 0;

    /// <summary>停止模块（关闭线程、释放资源）</summary>
    /// <param name="timeoutMs">等待超时毫秒数</param>
    virtual void stop(int timeoutMs = 3000) = 0;
};
