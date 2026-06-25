#pragma once

#include <QObject>
#include <memory>
#include <vector>
#include "IAppModule.h"

/// <summary>
/// 应用程序模块管理器——负责所有模块的创建、初始化、启动、停止。
/// TCV3 只需依赖此管理器，不再直接接触任何子系统。
/// </summary>
class AppModuleManager : public QObject
{
    Q_OBJECT

public:
    explicit AppModuleManager(QObject* parent = nullptr);
    ~AppModuleManager() override;

    /// <summary>注册一个模块（接管所有权）</summary>
    template<typename T, typename... Args>
    T* registerModule(Args&&... args)
    {
        auto module = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = module.get();
        m_modules.push_back(std::move(module));
        return ptr;
    }

    /// <summary>按类型获取已注册模块</summary>
    template<typename T>
    T* getModule()
    {
        for (auto& m : m_modules)
        {
            T* casted = dynamic_cast<T*>(m.get());
            if (casted) return casted;
        }
        return nullptr;
    }

    /// <summary>初始化所有已注册模块</summary>
    bool initializeAll();

    /// <summary>启动所有已注册模块</summary>
    void startAll();

    /// <summary>停止所有模块（逆序）</summary>
    void stopAll(int timeoutMs = 3000);

private:
    std::vector<std::unique_ptr<IAppModule>> m_modules;
};
