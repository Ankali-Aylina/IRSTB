#pragma once

#include <QDialog>
#include <QApplication>
#include <QMouseEvent>
#include "ui_ExitWidgets.h"

class ExitWidgets : public QDialog
{
	Q_OBJECT

public:
	ExitWidgets(QWidget* parent = nullptr);
	~ExitWidgets();

	/// 窗口拖动
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;
	virtual void mouseReleaseEvent(QMouseEvent* event) override;

public slots:

	//关闭提示窗口
	void TitleClose();
	//关闭所有窗口
	void ExitAllWeigts();
	//隐藏所有窗口
	void HideAllWeigts();

private:
	Ui::ExitWidgetsClass ui;

	bool m_leftMousePressed = false;
	QPointF m_StartPoint;
};
