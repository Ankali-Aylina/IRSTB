#include "ExitWidgets.h"

ExitWidgets::ExitWidgets(QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());

	//setWindowFlags(Qt::FramelessWindowHint);    //隐藏标题栏（无边框）
	setAttribute(Qt::WA_StyledBackground);      //启用样式背景绘制
	setAttribute(Qt::WA_TranslucentBackground); //背景透明
	setAttribute(Qt::WA_Hover);

	connect(ui.titlecloseButton, &QPushButton::clicked, this, &ExitWidgets::TitleClose);
	connect(ui.cancelButton, &QPushButton::clicked, this, &ExitWidgets::TitleClose);
	connect(ui.exitButton, &QPushButton::clicked, this, &ExitWidgets::ExitAllWeigts);
	connect(ui.minimizeButton, &QPushButton::clicked, this, &ExitWidgets::HideAllWeigts);
}

ExitWidgets::~ExitWidgets()
{
}

void ExitWidgets::TitleClose()
{
	this->close();
}

void ExitWidgets::ExitAllWeigts()
{
	QApplication::quit();
}

void ExitWidgets::HideAllWeigts()
{
	// 获取所有窗口
	QList<QWidget*> topLevelWidgets = QApplication::topLevelWidgets();
	for (QWidget* widget : topLevelWidgets) {
		// 检查窗口是否可以最小化
		if (widget->isWindow() && widget->isVisible()) {
			widget->hide();
		}
	}
}