#pragma once

#include <QObject>
#include <QMutex>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

class LogManagement : public QObject
{
	Q_OBJECT

public:
	LogManagement(QObject* parent);
	~LogManagement();

	enum LogLevel {
		LOG_DEBUG, ///调试信息
		LOG_INFO, ///消息信息
		LOG_WARNING, ///警告信息
		LOG_ERROR ///错误信息
	};

public slots:
	/// <summary>
	/// 日志信号
	/// </summary>
	/// <param name="message"> 日志信息 </param>
	/// <param name="level"> 日志等级 </param>>
	void logMessage(const QString& message, LogLevel level);

private:
	QMutex m_logMutex;
	QFile m_logFile;
	QString m_logPath = "app.log";
	void rotateLogs(); // 日志轮转
};
