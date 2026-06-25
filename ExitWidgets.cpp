#include "ExitWidgets.h"

ExitWidgets::ExitWidgets(QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | windowFlags());
	setWindowModality(Qt::ApplicationModal);

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

void ExitWidgets::mousePressEvent(QMouseEvent* event)
{
	// 允许在 titleBar 区域拖动窗口
	int x = ui.titleBar->x();
	int y = ui.titleBar->y();
	int w = ui.titleBar->width();
	int h = ui.titleBar->height();
	QPointF winpos = event->position();

	if (event->button() == Qt::LeftButton
		&& winpos.x() > x && winpos.x() < x + w
		&& winpos.y() > y && winpos.y() < y + h)
	{
		m_leftMousePressed = true;
		m_StartPoint = event->globalPosition();
	}
}

void ExitWidgets::mouseMoveEvent(QMouseEvent* event)
{
	if (m_leftMousePressed)
	{
		QPointF curPoint = event->globalPosition();
		QPointF movePoint = curPoint - m_StartPoint;
		this->move(this->pos().x() + movePoint.x(),
		           this->pos().y() + movePoint.y());
		m_StartPoint = curPoint;
	}
}

void ExitWidgets::mouseReleaseEvent(QMouseEvent* /*event*/)
{
	m_leftMousePressed = false;
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