#pragma once

#include <QVariant>
#include <QString>

/// <summary>
/// 配置提供者接口——解耦所有模块对 IniManagement 单例的硬依赖。
/// 生产环境使用 IniManagement（包装 QSettings），测试环境使用内存实现。
/// </summary>
class IConfigProvider
{
public:
    virtual ~IConfigProvider() = default;

    /// <summary>读取配置值</summary>
    virtual QVariant read(const QString& section, const QString& key) const = 0;

    /// <summary>写入配置值</summary>
    virtual void write(const QString& section, const QString& key, const QVariant& value) = 0;

    /// <summary>判断配置节是否已初始化</summary>
    virtual bool isInitialized(const QString& section) const = 0;

    /// <summary>初始化配置节（设置 InitStatus 标记）</summary>
    virtual void initSection(const QString& section, const QString& status) = 0;

    /// <summary>判断配置文件是否存在</summary>
    virtual bool fileExists() const = 0;

    /// <summary>删除配置文件</summary>
    virtual void deleteFile() = 0;
};
