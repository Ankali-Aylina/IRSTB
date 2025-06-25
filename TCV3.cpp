#include "TCV3.h"

TCV3::TCV3(QWidget* parent)
	: QMainWindow(parent),
	m_leftMousePressed(false),
	m_exitWidgets(nullptr)
{
	ui.setupUi(this);

	o_Log = new LogManagement(this);
	connect(this, &TCV3::logMessage, o_Log, &LogManagement::logMessage);

	//初始化配置文件
	initConfigFile();

	//初始化自启动界面
	autoStartUiUpdata();

	//初始化延时设置
	dataDelay();
	connect(ui.addDelayButton, &QPushButton::clicked, this, &TCV3::addDelay);
	connect(ui.minusButton, &QPushButton::clicked, this, &TCV3::minusDelay);

	setWindowFlags(Qt::FramelessWindowHint);    //隐藏标题栏（无边框）
	setAttribute(Qt::WA_StyledBackground);      //启用样式背景绘制
	setAttribute(Qt::WA_TranslucentBackground); //背景透明
	setAttribute(Qt::WA_Hover);

	setWindowIcon(QIcon(":/TCV3/res/icon/TreeNetWork.png")); // 设置窗口图标

	// 创建系统托盘图标
	trayIcon = new QSystemTrayIcon(this);
	QIcon icon(":/TCV3/res/icon/TrayIcon.png");
	trayIcon->setIcon(icon);
	// 设置鼠标悬停提示
	trayIcon->setToolTip("右键打开菜单");
	trayIcon->setVisible(true);

	// 创建托盘菜单
	QMenu* trayIconMenu = new QMenu(this);
	QAction* restoreAction = new QAction("恢复窗口", this);
	QAction* quitAction = new QAction("退出程序", this);

	//从托盘还原
	connect(restoreAction, &QAction::triggered, this, &TCV3::showNormal);
	connect(trayIcon, &QSystemTrayIcon::activated, this, &TCV3::showNormal);
	//退出程序
	connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

	//将动作添加到托盘菜单
	trayIconMenu->addAction(restoreAction);
	trayIconMenu->addAction(quitAction);
	trayIcon->setContextMenu(trayIconMenu);

	//设置版本号
	ui.aboutVersionLabel->setText(QString("V") + QString("3.3.2.3"));

	// 初始化菜单栏图标缓存
	m_buttonIcons = {
		{ QIcon(":/TCV3/res/icon/home_fill.png"), QIcon(":/TCV3/res/icon/home_line.png") },
		{ QIcon(":/TCV3/res/icon/settings_fill.png"), QIcon(":/TCV3/res/icon/settings_line.png") },
		{ QIcon(":/TCV3/res/icon/about_fill.png"), QIcon(":/TCV3/res/icon/about_line.png") }
	};

	// 初始化菜单栏按钮集合（顺序与stackedWidget索引对应）
	m_buttons = {
		ui.homeButton, // 0 index
		ui.settingsButton, // 1 index
		ui.aboutButton // 2 index
	};

	///图标刷新 （Lambda直接连接）
	auto createConnection = [this](QPushButton* btn, int index) {
		connect(btn, &QPushButton::clicked, this, [this, index]() {
			ui.stackedWidget->setCurrentIndex(index);
			updateButtonIcon();
			});
		};
	createConnection(ui.homeButton, 0);
	createConnection(ui.settingsButton, 1);
	createConnection(ui.aboutButton, 2);

	///窗口最小化隐藏到托盘
	connect(ui.minimizeButton, &QPushButton::clicked, this, &TCV3::hide);

	///窗口最大化
	connect(ui.maximizeButton, &QPushButton::clicked, this, &TCV3::showMaximizedOrNormal);

	///退出提示
	m_exitWidgets = new ExitWidgets();
	connect(ui.closeButton, &QPushButton::clicked, m_exitWidgets, &QDialog::open);

	///TC核心功能
	o_TCCore = new TCCore;
	q_TCCore = new QThread;
	o_TCCore->moveToThread(q_TCCore);
	q_TCCore->start();
	connect(q_TCCore, &QThread::started, o_TCCore, &TCCore::run);

	//温度更新ui
	connect(o_TCCore, &TCCore::cpuTemperatureUpdated, this, &TCV3::onCpuTemperatureUpdated);
	connect(o_TCCore, &TCCore::gpuTemperatureUpdated, this, &TCV3::onGpuTemperatureUpdated);

	///蓝牙功能
	o_BLEThread = new BLEThread;
	q_BLEThread = new QThread;
	o_BLEThread->moveToThread(q_BLEThread);
	q_BLEThread->start();
	connect(q_BLEThread, &QThread::started, o_BLEThread, &BLEThread::run);

	//数据发送
	m_fanControlConnection = connect(o_TCCore, &TCCore::controlDataUpdated, o_BLEThread, &BLEThread::controlFan);

	//重新连接蓝牙
	connect(ui.BLEButton, &QPushButton::clicked, o_BLEThread, &BLEThread::reconnectDevice);
	connect(ui.BLEButton, &QPushButton::clicked, this, &TCV3::bleReconnectUi);

	///蓝牙扫描更新蓝牙ui
	connect(o_BLEThread, &BLEThread::bleScanStarted, this, &TCV3::bleScanStarting);
	connect(o_BLEThread, &BLEThread::bleScanTimeout, this, &TCV3::bleScanTimeout);
	connect(o_BLEThread, &BLEThread::connectionInProgress, this, &TCV3::bleConnecting);
	connect(o_BLEThread, &BLEThread::connected, this, &TCV3::bleConnected);
	connect(o_BLEThread, &BLEThread::connectionFailed, this, &TCV3::bleConnectionFailed);
	connect(o_BLEThread, &BLEThread::disconnected, this, &TCV3::bleDisconnected);

	//自动模式
	connect(ui.AutoModeButton, &QPushButton::clicked, this, &TCV3::autoMode);
	connect(this, &TCV3::setFanAutoMode, o_BLEThread, &BLEThread::autoMode);

	//静音模式
	connect(ui.SilentModeButton, &QPushButton::clicked, this, &TCV3::silentMode);
	connect(this, &TCV3::setFanSilentMode, o_BLEThread, &BLEThread::silentMode);

	//全速模式
	connect(ui.PerformanceModeButton, &QPushButton::clicked, this, &TCV3::performanceMode);
	connect(this, &TCV3::setFanPerformanceMode, o_BLEThread, &BLEThread::performanceMode);

	//恢复配置
	connect(ui.resetSettingsButton, &QPushButton::clicked, this, &TCV3::resetSettings);

	//自启按钮
	connect(ui.autoStartButton, &QPushButton::clicked, this, &TCV3::autoStart);

	//数据延时更新按钮
	connect(ui.dataTxDelayButton, &QPushButton::clicked, this, &TCV3::setdataDelay);
	connect(this, &TCV3::setDataTrDelayUpdataFlag, o_TCCore, &TCCore::setDataTrDelayUpdataFlag);

	//支持按钮
	connect(ui.supportButton, &QPushButton::clicked, []() {
		QDesktopServices::openUrl(QUrl("https://ankali-aylina.github.io/2025/05/05/IRSTB/")); // 直接打开链接
		});

	//更新日志按钮
	m_updateLogDialog = new UpdateLogDialog(this);
	connect(ui.updataLogButton, &QPushButton::clicked, m_updateLogDialog, &QDialog::open);
}

TCV3::~TCV3()
{
	delete trayIcon;
	delete m_exitWidgets;
	delete m_updateLogDialog;
	delete o_TCCore;
	delete o_BLEThread;
	q_BLEThread->quit();
	q_BLEThread->wait(2000);
	delete q_BLEThread;
}

void TCV3::mousePressEvent(QMouseEvent* event)
{
	int x = ui.centralWidget->x();
	int y = ui.centralWidget->y();
	int w = ui.centralWidget->width();
	int h = ui.centralWidget->height();
	QPointF winpos;
	//只能是鼠标左键移动和改变大小
	if (event->button() == Qt::LeftButton) //处于左键状态
	{
		winpos = event->position();
		if ((winpos.x() > x && winpos.x() < x + w)  //x坐标位置判定
			&& (winpos.y() > y && winpos.y() < y + h)) //y坐标位置判定)
		{
			m_leftMousePressed = true;  //标志位置为真
			//按下时鼠标左键时，窗口在屏幕中的坐标
			m_StartPoint = event->globalPosition();
		}
	}
}

void TCV3::mouseMoveEvent(QMouseEvent* event)
{
	if (m_leftMousePressed)
	{
		QPointF curPoint = event->globalPosition();   //按住移动时的位置
		QPointF movePoint = curPoint - m_StartPoint; //与初始坐标做差，得位移
		//普通窗口
		QPoint mainWinPos = this->pos();
		//设置窗口的全局坐标
		this->move(mainWinPos.x() + movePoint.x(), mainWinPos.y() + movePoint.y());
		m_StartPoint = curPoint;
	}
}

void TCV3::mouseReleaseEvent(QMouseEvent* event)
{
	m_leftMousePressed = false;//释放鼠标，标志位置为假
}

bool TCV3::isAppRunning(const QString& lockName) {
	QString tempDir = QDir::tempPath();
	QLockFile lockFile(tempDir + "/" + lockName + ".lock");

	if (!lockFile.tryLock(100)) { // 100ms 尝试加锁
		return true;
	}

	return false;
}

///// <summary>
///// 更新显示页面
///// </summary>
//void TCV3::updateIndex()
//{
//	QPushButton* clickedButton = qobject_cast<QPushButton*>(sender());
//	if (clickedButton == ui.homeButton)
//	{
//		ui.stackedWidget->setCurrentIndex(0);
//	}
//	else if (clickedButton == ui.settingsButton)
//	{
//		ui.stackedWidget->setCurrentIndex(1);
//	}
//	else if (clickedButton == ui.aboutButton)
//	{
//		ui.stackedWidget->setCurrentIndex(2);
//	}
//	updateButtonIcon();
//}

/// <summary>
/// 更新菜单页按钮图标
/// </summary>
void TCV3::updateButtonIcon()
{
	/// 获取当前页面的索引
	const int currentIndex = ui.stackedWidget->currentIndex();

	for (int i = 0; i < m_buttons.size(); ++i) {
		m_buttons[i]->setIcon(i == currentIndex ?
			m_buttonIcons[i].active :
			m_buttonIcons[i].inactive);
	}

	//QIcon icon, icon_1, icon_2;
	//if (ui.stackedWidget->currentIndex() == 0)
	//{
	//	icon.addFile(QString::fromUtf8(":/TCV3/res/icon/home_fill.png"));
	//	ui.homeButton->setIcon(icon);

	//	icon_1.addFile(QString::fromUtf8(":/TCV3/res/icon/settings_line.png"));
	//	ui.settingsButton->setIcon(icon_1);

	//	icon_2.addFile(QString::fromUtf8(":/TCV3/res/icon/about_line.png"));
	//	ui.aboutButton->setIcon(icon_2);
	//}
	//else if (ui.stackedWidget->currentIndex() == 1)
	//{
	//	icon.addFile(QString::fromUtf8(":/TCV3/res/icon/settings_fill.png"));
	//	ui.settingsButton->setIcon(icon);

	//	icon_1.addFile(QString::fromUtf8(":/TCV3/res/icon/home_line.png"));
	//	ui.homeButton->setIcon(icon_1);
	//	icon_2.addFile(QString::fromUtf8(":/TCV3/res/icon/about_line.png"));
	//	ui.aboutButton->setIcon(icon_2);
	//}
	//else if (ui.stackedWidget->currentIndex() == 2)
	//{
	//	icon.addFile(QString::fromUtf8(":/TCV3/res/icon/about_fill.png"));
	//	ui.aboutButton->setIcon(icon);

	//	icon_1.addFile(QString::fromUtf8(":/TCV3/res/icon/home_line.png"));
	//	ui.homeButton->setIcon(icon_1);
	//	icon_2.addFile(QString::fromUtf8(":/TCV3/res/icon/settings_line.png"));
	//	ui.settingsButton->setIcon(icon_2);
	//}
}

/// <summary>
/// 最大化或还原窗口
/// </summary>
void TCV3::showMaximizedOrNormal()
{
	QIcon icon;
	if (this->isMaximized())
	{
		this->showNormal();

		icon.addFile(QString::fromUtf8(":/TCV3/res/icon/maximize.png"));
		ui.maximizeButton->setIcon(icon);
	}
	else
	{
		this->showMaximized();
		icon.addFile(QString::fromUtf8(":/TCV3/res/icon/normal.png"));
		ui.maximizeButton->setIcon(icon);
	}
}

/// <summary>
/// 退出提示
/// </summary>
void TCV3::showExitWidgets()
{
	m_exitWidgets->show();
}

void TCV3::bleReconnectUi()
{
	ui.BLELabel->setText("重连中...");
}

void TCV3::bleScanStarting()
{
	QIcon icon;
	icon.addFile(QString::fromUtf8(":/TCV3/res/icon/BLE_Off.png"));
	ui.BLEButton->setIcon(icon);
	ui.BLELabel->setText("搜索中...");
	ui.BLEProgressBar->setMaximum(0);
}

void TCV3::bleScanTimeout()
{
	QIcon icon;
	icon.addFile(QString::fromUtf8(":/TCV3/res/icon/BLE_Error.png"));
	ui.BLEButton->setIcon(icon);
	ui.BLELabel->setText("搜索超时");
	ui.BLEProgressBar->setMaximum(100);
	ui.BLEProgressBar->setValue(100);
}

void TCV3::bleConnecting()
{
	ui.BLELabel->setText("连接中...");
}

void TCV3::bleConnectionFailed()
{
	QIcon icon;
	icon.addFile(QString::fromUtf8(":/TCV3/res/icon/BLE_Error.png"));
	ui.BLEButton->setIcon(icon);
	ui.BLELabel->setText("连接失败");
	ui.BLELabel->setStyleSheet("color: #dc143c;");
	ui.BLEProgressBar->setMaximum(100);
	ui.BLEProgressBar->setValue(100);
}

void TCV3::bleConnected()
{
	QIcon icon;
	icon.addFile(QString::fromUtf8(":/TCV3/res/icon/BLE_On.png"));
	ui.BLEButton->setIcon(icon);
	ui.BLELabel->setText("连接完成");
	ui.BLELabel->setStyleSheet("color: white");
	ui.BLEProgressBar->setMaximum(100);
	ui.BLEProgressBar->setValue(0);
}

void TCV3::bleDisconnected()
{
	QIcon icon;
	icon.addFile(QString::fromUtf8(":/TCV3/res/icon/BLE_Error.png"));
	ui.BLEButton->setIcon(icon);
	ui.BLELabel->setText("断开连接");
	ui.BLELabel->setStyleSheet("color: #dc143c;");
	ui.BLEProgressBar->setMaximum(100);
	ui.BLEProgressBar->setValue(100);
}

void TCV3::onCpuTemperatureUpdated(int temperature)
{
	QIcon icon;
	if (temperature > 80) {
		icon.addFile(QString::fromUtf8(":/TCV3/res/icon/cpuTemp-red.png"));
		ui.cpuTempIcon->setIcon(icon);
		ui.cpuTempLabel->setStyleSheet("color: #dc143c;");
	}
	else {
		icon.addFile(QString::fromUtf8(":/TCV3/res/icon/cpuTemp.png"));
		ui.cpuTempIcon->setIcon(icon);
		ui.cpuTempLabel->setStyleSheet("color: white;");
	}

	ui.cpuTempProgressBar->setLayoutDirection(Qt::LeftToRight);
	ui.cpuTempProgressBar->setValue(100 - temperature);
	ui.cpuTempProgressBar->setMaximum(100);

	ui.cpuTempLabel->setText(QString::number(temperature) + "℃");
}

void TCV3::onGpuTemperatureUpdated(int temperature)
{
	QIcon icon;
	if (temperature > 75) {
		icon.addFile(QString::fromUtf8(":/TCV3/res/icon/gpuTemp-red.png"));
		ui.gpuTempIcon->setIcon(icon);
		ui.gpuTempLabel->setStyleSheet("color: #dc143c;");
	}
	else {
		icon.addFile(QString::fromUtf8(":/TCV3/res/icon/gpuTemp.png"));
		ui.gpuTempIcon->setIcon(icon);
		ui.gpuTempLabel->setStyleSheet("color: white;");
	}

	ui.gpuTempProgressBar->setLayoutDirection(Qt::LeftToRight);
	ui.gpuTempProgressBar->setValue(100 - temperature);
	ui.gpuTempProgressBar->setMaximum(100);

	ui.gpuTempLabel->setText(QString::number(temperature) + "℃");
}

void TCV3::autoMode()
{
	emit setFanAutoMode();
	QIcon SilentModeIcon, AutoModeIcon, PerformanceModeIcon;
	SilentModeIcon.addFile(QString::fromUtf8(":/TCV3/res/icon/Button_Off.png"));
	AutoModeIcon.addFile(QString::fromUtf8(":/TCV3/res/icon/Button_On.png"));
	PerformanceModeIcon.addFile(QString::fromUtf8(":/TCV3/res/icon/Button_Off.png"));

	ui.SilentModeButton->setIcon(SilentModeIcon);
	ui.AutoModeButton->setIcon(AutoModeIcon);
	ui.PerformanceModeButton->setIcon(PerformanceModeIcon);

	if (!m_fanControlConnection)
	{
		connect(o_TCCore, &TCCore::controlDataUpdated, o_BLEThread, &BLEThread::controlFan);
	}

	ui.SilentModeButton->setEnabled(true);
	ui.AutoModeButton->setEnabled(false);
	ui.PerformanceModeButton->setEnabled(true);
}

void TCV3::silentMode()
{
	QIcon SilentModeIcon, AutoModeIcon, PerformanceModeIcon;
	SilentModeIcon.addFile(QString::fromUtf8(":/TCV3/res/icon/Button_On.png"));
	AutoModeIcon.addFile(QString::fromUtf8(":/TCV3/res/icon/Button_Off.png"));
	PerformanceModeIcon.addFile(QString::fromUtf8(":/TCV3/res/icon/Button_Off.png"));

	ui.SilentModeButton->setIcon(SilentModeIcon);
	ui.AutoModeButton->setIcon(AutoModeIcon);
	ui.PerformanceModeButton->setIcon(PerformanceModeIcon);

	if (m_fanControlConnection)
	{
		disconnect(o_TCCore, &TCCore::controlDataUpdated, o_BLEThread, &BLEThread::controlFan);
	}

	emit setFanSilentMode();

	ui.SilentModeButton->setEnabled(false);
	ui.AutoModeButton->setEnabled(true);
	ui.PerformanceModeButton->setEnabled(true);
}

void TCV3::performanceMode()
{
	QIcon SilentModeIcon, AutoModeIcon, PerformanceModeIcon;
	SilentModeIcon.addFile(QString::fromUtf8(":/TCV3/res/icon/Button_Off.png"));
	AutoModeIcon.addFile(QString::fromUtf8(":/TCV3/res/icon/Button_Off.png"));
	PerformanceModeIcon.addFile(QString::fromUtf8(":/TCV3/res/icon/Button_On.png"));

	ui.SilentModeButton->setIcon(SilentModeIcon);
	ui.AutoModeButton->setIcon(AutoModeIcon);
	ui.PerformanceModeButton->setIcon(PerformanceModeIcon);

	if (m_fanControlConnection)
	{
		disconnect(o_TCCore, &TCCore::controlDataUpdated, o_BLEThread, &BLEThread::controlFan);
	}

	emit setFanPerformanceMode();

	ui.SilentModeButton->setEnabled(true);
	ui.AutoModeButton->setEnabled(true);
	ui.PerformanceModeButton->setEnabled(false);
}

void TCV3::resetSettings()
{
	// 弹出确认对话框
	QMessageBox::StandardButton reply = QMessageBox::question(
		this,
		"重启确认",
		"删除配置会重启应用，是否继续？",
		QMessageBox::Yes | QMessageBox::No
	);

	if (reply == QMessageBox::Yes) {
		// 启动新实例并关闭当前程序
		IniManagement::instance().deleteIniFile();
		QProcess::startDetached(QApplication::applicationFilePath());
		QApplication::quit(); // 关闭当前程序
	}
}

void TCV3::initConfigFile()
{
	if (!IniManagement::instance().IsInit("UI"))
	{
		IniManagement::instance().InitSection("UI", "false");

		IniManagement::instance().write("UI", "dataTxDelay", "5");

		IniManagement::instance().InitSection("UI", "true");
	}
}

void TCV3::autoStart()
{
	if (!isAutoStartEnabled())
	{
		setAutoStart(true);
		IniManagement::instance().write("UI", "AutoStart", "true");
	}
	else
	{
		setAutoStart(false);
		IniManagement::instance().write("UI", "AutoStart", "false");
	}
	autoStartUiUpdata();
}

void TCV3::autoStartUiUpdata()
{
	if (isAutoStartEnabled())
	{
		QIcon icon;
		icon.addFile(QString::fromUtf8(":/TCV3/res/icon/switch-on.png"));
		ui.autoStartButton->setIcon(icon);
	}
	else
	{
		QIcon icon;
		icon.addFile(QString::fromUtf8(":/TCV3/res/icon/switch-off.png"));
		ui.autoStartButton->setIcon(icon);
	}

	//if (IniManagement::instance().read("UI", "AutoStart") == QString("true"))
	//{
	//	QIcon icon;
	//	icon.addFile(QString::fromUtf8(":/TCV3/res/icon/switch-on.png"));
	//	ui.autoStartButton->setIcon(icon);
	//}
}

void TCV3::setAutoStart(bool enable)
{
	QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
	QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());

	// 添加双引号以处理路径中的空格
	appPath = "\"" + appPath + "\"";

	if (enable) {
		settings.setValue("TemperatureControlV3", appPath);
	}
	else {
		settings.remove("TemperatureControlV3");
	}
}

bool TCV3::isAutoStartEnabled()
{
	QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
	QString regValue = settings.value("TemperatureControlV3").toString().trimmed(); // 去除首尾空格

	// 处理路径引号：若注册表值被双引号包裹，则去除引号
	if (regValue.startsWith('"') && regValue.endsWith('"')) {
		regValue = regValue.mid(1, regValue.length() - 2);
	}

	QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());

	return settings.contains("TemperatureControlV3") && (regValue == appPath);
}

void TCV3::dataDelay()
{
	int uiDelay = IniManagement::instance().read("UI", "dataTxDelay").toInt();

	if (uiDelay > 0 && uiDelay < 11)
	{
		ui.dataDelayLabel->setText(QString::number(uiDelay));
	}
	else {
		emit logMessage(QString("超出调节范围！重置为初始值！"), LogManagement::LOG_ERROR);
		IniManagement::instance().write("UI", "dataTxDelay", "5");
		uiDelay = IniManagement::instance().read("UI", "dataTxDelay").toInt();
		ui.dataDelayLabel->setText(QString::number(uiDelay));
	}
}

void TCV3::setdataDelay()
{
	int uiDelay = ui.dataDelayLabel->text().toInt();
	int dataDelay = IniManagement::instance().read("TC", "DataTransmissionDelay").toInt();
	if (uiDelay == dataDelay / 1000)
	{
		return;
	}
	else
	{
		IniManagement::instance().write("TC", "DataTransmissionDelay", QString::number(uiDelay * 1000));

		emit  setDataTrDelayUpdataFlag(true);

		emit logMessage(QString("设置延时为%1s").arg(uiDelay), LogManagement::LOG_INFO);
	}
}

void TCV3::dataDelayUiUpdate(int data)
{
	if (data > 0 && data < 11)
	{
		ui.dataDelayLabel->setText(QString::number(data));
	}
	else {
		emit logMessage(QString("超出调节范围！"), LogManagement::LOG_ERROR);
	}
}

void TCV3::addDelay()
{
	int dataDelay = IniManagement::instance().read("UI", "dataTxDelay").toInt();
	if (dataDelay < 11)
	{
		dataDelay++;
		IniManagement::instance().write("UI", "dataTxDelay", QString::number(dataDelay));
		dataDelayUiUpdate(dataDelay);
	}
	else
	{
		emit logMessage(QString("超出调节范围！"), LogManagement::LOG_ERROR);
	}
}

void TCV3::minusDelay()
{
	int dataDelay = IniManagement::instance().read("UI", "dataTxDelay").toInt();
	if (dataDelay > 0)
	{
		dataDelay--;
		IniManagement::instance().write("UI", "dataTxDelay", QString::number(dataDelay));
		dataDelayUiUpdate(dataDelay);
	}
	else
	{
		emit logMessage(QString("超出调节范围！"), LogManagement::LOG_ERROR);
	}
}