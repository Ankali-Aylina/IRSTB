#pragma once

#include <QDialog>
#include <QApplication>
#include "ui_ExitWidgets.h"

class ExitWidgets : public QDialog
{
	Q_OBJECT

public:
	ExitWidgets(QWidget* parent = nullptr);
	~ExitWidgets();

public slots:

	//关闭提示窗口
	void TitleClose();
	//关闭所有窗口
	void ExitAllWeigts();
	//隐藏所有窗口
	void HideAllWeigts();

private:
	Ui::ExitWidgetsClass ui;
};
