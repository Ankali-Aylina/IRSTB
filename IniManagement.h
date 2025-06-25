#pragma once

#include <QObject>
#include <QSettings>
#include <QFile>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMutex>

#include <LogManagement.h>

class IniManagement : public QObject
{
	Q_OBJECT

public:
	static IniManagement& instance();
	~IniManagement();

	/// <summary>
	/// ini文件数据
	/// </summary>
	struct IniFile_Data
	{
		/// <summary>
		/// ini文件状态
		/// </summary>
		int IniFile_Status = 0;

		/// <summary>
		/// ini文件路径
		/// </summary>
		QString IniFilePath;
	};
	IniFile_Data IniFileData;

private:
	IniManagement(QObject* parent = nullptr); // 私有构造函数

	QScopedPointer<QSettings> m_settings; // 使用智能指针管理 QSettings
	QString m_path;
	LogManagement* m_logIniMgmt;

	/*mutable 的作用：允许在 const 成员函数中修改成员变量。
	线程安全：QMutex 的锁定和解锁属于“物理状态”修改，
	不影响对象逻辑状态，因此使用 mutable 是安全的。*/
	mutable QMutex m_iniMutex;

public slots:
	/// <summary>
	/// 写入配置文件
	/// </summary>
	/// <param name="section"> 配置文件中的节名 </param>
	/// <param name="key"> 配置文件中的键名 </param>
	/// <param name="value"> 配置文件中的键值 </param>
	void write(const QString& section, const QString& key, const QVariant& value);

	/// <summary>
	/// 读取配置文件
	/// </summary>
	/// <param name="section"> 配置文件中的节名 </param>
	/// <param name="key"> 配置文件中的键名 </param>
	/// <returns> 配置文件中的键值 </returns>
	QVariant read(const QString& section, const QString& key) const;

	/// <summary>
	/// 初始化配置文件节
	/// </summary>
	/// <param name="section"> 配置文件中的节名</param>
	/// <param name="status"> 配置文件节的状态 </param>
	void InitSection(const QString& section, const QString& status);

	/// <summary>
	/// 判断ini文件是否初始化
	/// </summary>
	/// <param name="iniFilePath"> ini文件路径 </param>
	/// <returns></returns>
	bool IsInit(const QString& section) const;

	/// <summary>
	/// 判断ini文件是否存在
	/// </summary>
	/// <param name="iniFilePath"> ini文件路径 </param>
	/// <returns></returns>
	bool IsExist() const;

	/// <summary>
	/// 删除ini文件
	/// </summary>
	void deleteIniFile();

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
