#include "IniManagement.h"

IniManagement::IniManagement(QObject* parent)
{
	m_path = QCoreApplication::applicationDirPath() + "./config.ini";
	m_settings.reset(new QSettings(m_path, QSettings::IniFormat));

	m_logIniMgmt = new LogManagement(this);
	connect(this, &IniManagement::logMessage, m_logIniMgmt, &LogManagement::logMessage);
}

IniManagement& IniManagement::instance()
{
	static IniManagement instance;
	return instance;
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
	m_settings->sync(); // 同步写入文件

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

void IniManagement::InitSection(const QString& section, const QString& status)
{
	write(section, "InitStatus", status);
}

bool IniManagement::IsInit(const QString& section) const
{
	QString IniFile_InitStatus = read(section, "InitStatus").toString();
	if (IniFile_InitStatus != "true" || IniFile_InitStatus == "")
	{
		return false;
	}
	return true;
}

bool IniManagement::IsExist() const
{
	QFile file(m_path);
	bool exists = file.exists();
	emit logMessage(
		QString("INI文件%1").arg(exists ? "存在" : "不存在"),
		LogManagement::LogLevel::LOG_INFO
	);
	return exists;
}

void IniManagement::deleteIniFile()
{
	QFile file(m_path);
	if (file.exists()) {
		file.remove();
	}
}

void IniManagement::setPath(const QString& path)
{
	if (m_path != path) {
		m_path = path;
		m_settings.reset(new QSettings(m_path, QSettings::IniFormat));
	}
}