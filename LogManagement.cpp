#include "LogManagement.h"

#include <QDir>
#include <QFileInfo>

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

	// 重命名当前日志为带时间戳的备份（仅保留最近 1 份 + 当前 = 共 2 份）
	QString backupName = QDateTime::currentDateTime().toString("'app_'yyyyMMdd_hhmmss'.log'");
	QString backupPath = QFileInfo(m_logPath).absolutePath() + "/" + backupName;

	if (!QFile::rename(m_logPath, backupPath)) {
		qWarning() << "Log rotation rename failed";
	}

	// 删除多余旧备份，仅保留最近 1 份
	QDir logDir = QFileInfo(m_logPath).absoluteDir();
	QStringList oldLogs = logDir.entryList({"app_*.log"}, QDir::Files, QDir::Name | QDir::Reversed);
	for (int i = 1; i < oldLogs.size(); ++i) {
		QFile::remove(logDir.filePath(oldLogs[i]));
	}

	(void)m_logFile.open(QIODevice::Append | QIODevice::Text);
}