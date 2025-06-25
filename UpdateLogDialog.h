#pragma once

#include <QDialog>
#include <QSizeGrip>
#include <QMouseEvent>
#include "LogManagement.h"
#include "ui_UpdateLogDialog.h"

class UpdateLogDialog : public QDialog
{
	Q_OBJECT

public:
	UpdateLogDialog(QWidget* parent = nullptr);
	~UpdateLogDialog();

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

private:
	Ui::UpdateLogDialogClass ui;
	QString mdFile;
	LogManagement* m_log;

	QSizeGrip* sizeGrip;
	enum ResizeRegion { None, Right, Bottom, BottomRight };
	bool m_bDragging;
	bool m_bResizing;
	ResizeRegion m_resizeRegion;
	QPoint m_dragPosition;
	QPoint m_dragStartPosition;

signals:
	/// <summary>
	/// 日志信号
	/// </summary>
	/// <param name="message"></param>
	/// <param name="level"></param>
	void logMessage(const QString& message, LogManagement::LogLevel level);
};
