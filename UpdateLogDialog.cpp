#include "UpdateLogDialog.h"
#include <Qfile>

UpdateLogDialog::UpdateLogDialog(QWidget* parent)
	: QDialog(parent),
	m_bDragging(false),
	m_bResizing(false),
	m_resizeRegion(None)
{
	ui.setupUi(this);

	this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());

	//setWindowFlags(Qt::FramelessWindowHint);    //隐藏标题栏（无边框）
	setAttribute(Qt::WA_StyledBackground);      //启用样式背景绘制
	setAttribute(Qt::WA_TranslucentBackground); //背景透明
	setAttribute(Qt::WA_Hover);

	sizeGrip = new QSizeGrip(this);
	// 设置QSizeGrip的背景颜色为透明
	sizeGrip->setStyleSheet("background-color: transparent;");
	// 设置QSizeGrip的大小为16x16
	sizeGrip->setFixedSize(32, 32);
	// 将QSizeGrip移动到窗口的右下角
	sizeGrip->move(width() - sizeGrip->width(), height() - sizeGrip->height());

	connect(ui.closeButton, &QPushButton::clicked, this, &UpdateLogDialog::close);

	// 连接日志到全局单例
	connect(this, &UpdateLogDialog::logMessage, &LogManagement::instance(), &LogManagement::logMessage);

	mdFile = QCoreApplication::applicationDirPath() + "/updatalog.md";
	QFile file(mdFile);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qWarning() << "无法打开文件:" << mdFile;
		emit logMessage(QString("无法打开文件" + mdFile), LogManagement::LogLevel::LOG_ERROR);
		return;
	}

	QTextStream stream(&file);
	QString markdownText = stream.readAll();
	file.close();
	ui.textBrowser->setMarkdown(markdownText);
}

UpdateLogDialog::~UpdateLogDialog()
{
}

// 鼠标按下事件
void UpdateLogDialog::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		if (m_resizeRegion != None) {
			m_bResizing = true;
			m_dragStartPosition = event->globalPosition().toPoint();
		}
		else {
			m_bDragging = true;
			m_dragPosition = event->globalPosition().toPoint() - this->pos();
		}
		event->accept();
	}
	QDialog::mousePressEvent(event);
}

// 鼠标移动事件
void UpdateLogDialog::mouseMoveEvent(QMouseEvent* event) {
	QPoint pos = event->pos();
	int edgeMargin = 5;

	// 检测鼠标是否在边缘区域
	if (!m_bDragging && !m_bResizing) {
		if (pos.x() >= width() - edgeMargin && pos.y() >= height() - edgeMargin) {
			setCursor(Qt::SizeFDiagCursor);
			m_resizeRegion = BottomRight;
		}
		else if (pos.x() >= width() - edgeMargin) {
			setCursor(Qt::SizeHorCursor);
			m_resizeRegion = Right;
		}
		else if (pos.y() >= height() - edgeMargin) {
			setCursor(Qt::SizeVerCursor);
			m_resizeRegion = Bottom;
		}
		else {
			setCursor(Qt::ArrowCursor);
			m_resizeRegion = None;
		}
	}

	if (m_bResizing) {
		QPoint delta = event->globalPosition().toPoint() - m_dragStartPosition;
		QRect newGeo = geometry();
		switch (m_resizeRegion) {
		case Right:
			newGeo.setWidth(newGeo.width() + delta.x());
			break;
		case Bottom:
			newGeo.setHeight(newGeo.height() + delta.y());
			break;
		case BottomRight:
			newGeo.setWidth(newGeo.width() + delta.x());
			newGeo.setHeight(newGeo.height() + delta.y());
			break;
		default:
			break;
		}
		if (newGeo.width() < minimumWidth()) newGeo.setWidth(minimumWidth());
		if (newGeo.height() < minimumHeight()) newGeo.setHeight(minimumHeight());
		setGeometry(newGeo);
		m_dragStartPosition = event->globalPosition().toPoint();
	}
	else if (m_bDragging) {
		move(event->globalPosition().toPoint() - m_dragPosition);
		event->accept();
	}
	QDialog::mouseMoveEvent(event);
}

// 鼠标释放事件
void UpdateLogDialog::mouseReleaseEvent(QMouseEvent* event) {
	m_bDragging = false;
	m_bResizing = false;
	sizeGrip->move(width() - sizeGrip->width(), height() - sizeGrip->height());
	QDialog::mouseReleaseEvent(event);
}