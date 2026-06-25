#pragma once

#include <QObject>
#include <QSettings>
#include <QFile>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMutex>

#include <LogManagement.h>
#include "IConfigProvider.h"

class IniManagement : public QObject, public IConfigProvider
{
	Q_OBJECT

public:
	static IniManagement& instance();
	explicit IniManagement(QObject* parent = nullptr);
	~IniManagement();

	QScopedPointer<QSettings> m_settings; // 使用智能指针管理 QSettings
	QString m_path;

	/*mutable 的作用：允许在 const 成员函数中修改成员变量。
	线程安全：QMutex 的锁定和解锁属于“物理状态”修改，
	不影响对象逻辑状态，因此使用 mutable 是安全的。*/
	mutable QMutex m_iniMutex;

public slots:
	/// <summary>写入配置文件（IConfigProvider 实现）</summary>
	void write(const QString& section, const QString& key, const QVariant& value) override;

	/// <summary>读取配置文件（IConfigProvider 实现）</summary>
	QVariant read(const QString& section, const QString& key) const override;

	/// <summary>初始化配置节</summary>
	void initSection(const QString& section, const QString& status) override;

	/// <summary>判断配置节是否已初始化</summary>
	bool isInitialized(const QString& section) const override;

	/// <summary>判断配置文件是否存在</summary>
	bool fileExists() const override;

	/// <summary>删除配置文件</summary>
	void deleteFile() override;

	/// <summary>
	/// 设置ini文件路径
	/// </summary>
	/// <param name="path"></param>
	void setPath(const QString& path);

signals:
	/// <summary>
	/// 日志信号
	/// </summary>
	/// <param name="message"></param>
	/// <param name="level"></param>
	void logMessage(const QString& message, LogManagement::LogLevel level) const;
};
