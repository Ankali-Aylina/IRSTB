#include "IniManagement.h"

IniManagement::IniManagement(QObject* parent)
	: QObject(parent)
{
	m_path = QCoreApplication::applicationDirPath() + "/config.ini";
	m_settings.reset(new QSettings(m_path, QSettings::IniFormat));

	connect(this, &IniManagement::logMessage, &LogManagement::instance(), &LogManagement::logMessage);
}

IniManagement::~IniManagement()
{
}

void IniManagement::write(const QString& section, const QString& key, const QVariant& value)
{
	QMutexLocker locker(&m_iniMutex);

	m_settings->beginGroup(section);
	m_settings->setValue(key, value);
	m_settings->endGroup();
	// 移除每次写入的 sync()——QSettings 在析构时自动同步，或调用 flush() 批量刷新

	if (m_settings->status() != QSettings::NoError)
	{
		emit logMessage(QString("配置文件写入错误！"), LogManagement::LogLevel::LOG_ERROR);
	}
}

QVariant IniManagement::read(const QString& section, const QString& key) const
{
	QMutexLocker locker(&m_iniMutex);
	m_settings->beginGroup(section);
	QVariant value = m_settings->value(key);
	m_settings->endGroup();
	return value;
}

void IniManagement::initSection(const QString& section, const QString& status)
{
	write(section, "InitStatus", status);
}

bool IniManagement::isInitialized(const QString& section) const
{
	QString IniFile_InitStatus = read(section, "InitStatus").toString();
	return IniFile_InitStatus == "true";
}

bool IniManagement::fileExists() const
{
	QFile file(m_path);
	bool exists = file.exists();
	emit logMessage(
		QString("INI文件%1").arg(exists ? "存在" : "不存在"),
		LogManagement::LogLevel::LOG_INFO
	);
	return exists;
}

void IniManagement::deleteFile()
{
	// QSettings 是惰性写入：所有 setValue 都缓存在内存，析构/sync() 时才落盘。
	// 必须先清空内存缓存并落盘，再删除文件——否则 QSettings 析构时会
	// 把旧值重新写回磁盘，导致"重置配置"功能失效。
	m_settings->clear();
	m_settings->sync();

	QFile file(m_path);
	if (file.exists()) {
		file.remove();
	}
}

bool IniManagement::isFirstRun() const
{
	// 优先检查统一标记
	if (isInitialized("App"))
		return false;

	// 向后兼容：如果旧版各个段都已初始化，视为非首次运行
	if (isInitialized("TC") || isInitialized("UI"))
		return false;

	return true;
}

void IniManagement::markFirstRunDone()
{
	initSection("App", "true");
}

void IniManagement::setPath(const QString& path)
{
	if (m_path != path) {
		m_path = path;
		m_settings.reset(new QSettings(m_path, QSettings::IniFormat));
	}
}