#include "LogManagement.h"

LogManagement& LogManagement::instance()
{
	static LogManagement inst;
	return inst;
}

LogManagement::LogManagement(QObject* parent)
	: QObject(parent)
{
}

LogManagement::~LogManagement()
{
}

void LogManagement::logMessage(const QString& message, LogLevel level) {
	QMutexLocker locker(&m_logMutex);

	// 添加时间戳和日志级别
	QString levelStr;
	switch (level) {
	case LOG_DEBUG: levelStr = "DEBUG"; break;
	case LOG_INFO: levelStr = "INFO"; break;
	case LOG_WARNING: levelStr = "WARN"; break;
	case LOG_ERROR: levelStr = "ERROR"; break;
	}

	if (levelStr != "DEBUG") {
		QString logMsg = QString("[%1][%2] %3")
			.arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
			.arg(levelStr)
			.arg(message);
	

	// 部署到发布环境取消debug级别的日志输出
	//QString logMsg = QString("[%1][%2] %3")
	//	.arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
	//	.arg(levelStr)
	//	.arg(message);

	// 控制台输出
	qDebug().noquote() << logMsg;

	// 文件输出
	if (!m_logFile.isOpen()) {
		m_logFile.setFileName(m_logPath);
		(void)m_logFile.open(QIODevice::Append | QIODevice::Text);
	}
	if (m_logFile.size() > 5 * 1024 * 1024) { // 5MB轮转
		rotateLogs();
	}
	QTextStream stream(&m_logFile);
	stream << logMsg << "\n";
	stream.flush();
	}
}

void LogManagement::rotateLogs() {
	m_logFile.close();
	if (!QFile::rename(m_logPath,
		QDateTime::currentDateTime().toString("'app_'yyyyMMdd_hhmmss'.log'")))
	{
		qWarning() << "Log rotation rename failed";
	}
	(void)m_logFile.open(QIODevice::Append | QIODevice::Text);
}