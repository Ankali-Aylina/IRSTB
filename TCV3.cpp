#include "TCV3.h"
#include "ApplicationBootstrap.h"
#include <QDesktopServices>
#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QUrl>
#include <windows.h>

TCV3::TCV3(QWidget* parent)
	: QMainWindow(parent),
	m_leftMousePressed(false),
	m_exitWidgets(nullptr),
	m_moduleManager(nullptr)
{
	ui.setupUi(this);

	// --- 现代化全局样式表 ---
	qApp->setStyleSheet(R"(
		/* ========== 全局基础 ========== */
		* {
			font-family: "MiSans", "Microsoft YaHei", "Segoe UI", sans-serif;
		}

		QToolTip {
			background: #2a3040;
			color: #e0e0e0;
			border: 1px solid #5b8ec4;
			border-radius: 4px;
			padding: 4px 8px;
			font-size: 12px;
		}

		/* ========== 滚动条 ========== */
		QScrollBar:vertical {
			background: #1e2d3d;
			width: 8px;
			border-radius: 4px;
		}
		QScrollBar::handle:vertical {
			background: #5b8ec4;
			border-radius: 4px;
			min-height: 20px;
		}
		QScrollBar::handle:vertical:hover {
			background: #6ea3d9;
		}
		QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
			height: 0;
		}

		/* ========== 按钮通用 ========== */
		QPushButton {
			color: #e8e8e8;
			background: #5b8ec4;
			border: none;
			border-radius: 6px;
			padding: 4px 12px;
		}
		QPushButton:hover {
			background: #6ea3d9;
		}
		QPushButton:pressed {
			background: #3d5575;
		}

		/* ========== 标签 ========== */
		QLabel {
			color: #e8e8e8;
			background: transparent;
		}

		/* ========== 进度条通用 ========== */
		QProgressBar {
			border: none;
			border-radius: 6px;
			text-align: center;
			color: rgba(255,255,255,0);
			background: #1a2d3f;
			height: 24px;
		}
		QProgressBar::chunk {
			border-radius: 6px;
			background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
				stop:0 #4ecdc4, stop:0.5 #ffd93d, stop:1 #e04050);
		}

		/* ========== 输入框 ========== */
		QLineEdit, QTextEdit, QPlainTextEdit {
			background: #1a2d3f;
			color: #e8e8e8;
			border: 1px solid #3d5575;
			border-radius: 6px;
			padding: 6px 10px;
			selection-background-color: #5b8ec4;
		}
		QLineEdit:focus, QTextEdit:focus {
			border-color: #5b8ec4;
		}

		/* ========== 消息框 ========== */
		QMessageBox {
			background: #2a3f5f;
			color: #e8e8e8;
		}
		QMessageBox QLabel {
			color: #e8e8e8;
		}
		QMessageBox QPushButton {
			min-width: 80px;
			min-height: 28px;
			background: #5b8ec4;
			border-radius: 6px;
			color: #e8e8e8;
		}
		QMessageBox QPushButton:hover {
			background: #6ea3d9;
		}
	)");

	// --- 日志管理（全局单例） ---
	connect(this, &TCV3::logMessage, &LogManagement::instance(), &LogManagement::logMessage);

	// --- 配置提供者（替代全局单例，注入到所有子系统） ---
	auto* iniConfig = new IniManagement(this);
	m_config = iniConfig;

	// --- 模块管理器：统一创建、初始化、启动所有子系统 ---
	m_moduleManager = new AppModuleManager(this);

	auto* tcc = m_moduleManager->registerModule<TCCore>(m_config);
	auto* ble = m_moduleManager->registerModule<BLEThread>(m_config);

	m_moduleManager->initializeAll();
	m_moduleManager->startAll();

	// --- 跨模块连接：TCCore 输出控制数据 -> BLE 风扇控制 ---
	m_fanControlConnection = connect(tcc, &TCCore::controlDataUpdated,
	                                  ble, &BLEThread::controlFan,
	                                  Qt::QueuedConnection);

	// TCCore 连接状态检测 -> BLE 连接状态更新
	connect(tcc, &TCCore::updateConnectionStatus, ble, &BLEThread::updateConnectionStatus,
	        Qt::QueuedConnection);

	// --- 模块 -> UI 连接 ---
	connect(tcc, &TCCore::cpuTemperatureUpdated, this, &TCV3::onCpuTemperatureUpdated);
	connect(tcc, &TCCore::gpuTemperatureUpdated, this, &TCV3::onGpuTemperatureUpdated);

	// --- 蓝牙状态呈现器 ---
	m_blePresenter = new BluetoothStatusPresenter(
		ui.BLELabel, ui.BLEButton, ui.BLEProgressBar,
		QIcon(":/TCV3/res/icon/BLE_On.png"),
		QIcon(":/TCV3/res/icon/BLE_Off.png"),
		QIcon(":/TCV3/res/icon/BLE_Error.png"),
		this);

	connect(ble, &BLEThread::bleScanStarted, m_blePresenter, &BluetoothStatusPresenter::onScanStarted);
	connect(ble, &BLEThread::bleScanTimeout, m_blePresenter, &BluetoothStatusPresenter::onScanTimeout);
	connect(ble, &BLEThread::connectionInProgress, m_blePresenter, &BluetoothStatusPresenter::onConnecting);
	connect(ble, &BLEThread::connected, m_blePresenter, &BluetoothStatusPresenter::onConnected);
	connect(ble, &BLEThread::connectionFailed, m_blePresenter, &BluetoothStatusPresenter::onConnectionFailed);
	connect(ble, &BLEThread::disconnected, m_blePresenter, &BluetoothStatusPresenter::onDisconnected);

	// --- UI -> 模块 连接（风扇模式） ---
	connect(this, &TCV3::setFanAutoMode, ble, &BLEThread::autoMode);
	connect(this, &TCV3::setFanSilentMode, ble, &BLEThread::silentMode);
	connect(this, &TCV3::setFanPerformanceMode, ble, &BLEThread::performanceMode);

	// --- UI -> 模块 连接（延时设置） ---
	connect(this, &TCV3::setDataTrDelayUpdataFlag, tcc, &TCCore::setDataTrDelayUpdataFlag);

	// --- 以下为纯 UI 初始化 ---
	initConfigFile();
	preloadIcons();

	// 加载温度警告阈值（与 TemperatureConfig 保持同步）
	{
		QVariant v = m_config->read("TC", "WarningCpu");
		if (v.isValid()) m_warningCpuThreshold = v.toInt();
		v = m_config->read("TC", "WarningGpu");
		if (v.isValid()) m_warningGpuThreshold = v.toInt();
	}

	autoStartUiUpdata();
	dataDelay();

	connect(ui.addDelayButton, &QPushButton::clicked, this, &TCV3::addDelay);
	connect(ui.minusButton, &QPushButton::clicked, this, &TCV3::minusDelay);

	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_StyledBackground);
	setAttribute(Qt::WA_TranslucentBackground);
	setAttribute(Qt::WA_Hover);
	setWindowIcon(QIcon(":/TCV3/res/icon/TreeNetWork.png"));

	// 系统托盘
	trayIcon = new QSystemTrayIcon(this);
	QIcon icon(":/TCV3/res/icon/TrayIcon.png");
	trayIcon->setIcon(icon);
	trayIcon->setToolTip("右键打开菜单");
	trayIcon->setVisible(true);

	QMenu* trayIconMenu = new QMenu(this);
	QAction* restoreAction = new QAction("恢复窗口", this);
	QAction* quitAction = new QAction("退出程序", this);
	connect(restoreAction, &QAction::triggered, this, &TCV3::showNormal);
	connect(trayIcon, &QSystemTrayIcon::activated, this, &TCV3::showNormal);
	connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
	trayIconMenu->addAction(restoreAction);
	trayIconMenu->addAction(quitAction);
	trayIcon->setContextMenu(trayIconMenu);

	ui.aboutVersionLabel->setText(QString("V") + QCoreApplication::applicationVersion());

	// --- 页面切换动画初始化 ---
	m_pageOpacityEffect = new QGraphicsOpacityEffect(this);
	m_pageOpacityEffect->setOpacity(1.0);
	ui.stackedWidget->setGraphicsEffect(m_pageOpacityEffect);

	// 菜单栏按钮
	m_buttons = { ui.homeButton, ui.settingsButton, ui.aboutButton };
	m_fanModeButtons = { ui.AutoModeButton, ui.SilentModeButton, ui.PerformanceModeButton };
	auto createConnection = [this](QPushButton* btn, int index) {
		connect(btn, &QPushButton::clicked, this, [this, index]() {
			animatePageSwitch(index);
		});
	};
	createConnection(ui.homeButton, 0);
	createConnection(ui.settingsButton, 1);
	createConnection(ui.aboutButton, 2);

	connect(ui.minimizeButton, &QPushButton::clicked, this, [this]() {
		hide();
		// 最小化时裁剪工作集，释放物理内存（虚拟内存不变，页面按需换回）
		SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
	});
	connect(ui.maximizeButton, &QPushButton::clicked, this, &TCV3::showMaximizedOrNormal);

	// 退出提示
	m_exitWidgets = new ExitWidgets();
	connect(ui.closeButton, &QPushButton::clicked, m_exitWidgets, &QDialog::open);

	// 蓝牙重连
	connect(ui.BLEButton, &QPushButton::clicked, ble, &BLEThread::reconnectDevice);
	connect(ui.BLEButton, &QPushButton::clicked, m_blePresenter, &BluetoothStatusPresenter::onReconnect);
	connect(ui.BLEButton, &QPushButton::clicked, this, &TCV3::autoMode);

	// 风扇模式按钮
	connect(ui.AutoModeButton, &QPushButton::clicked, this, &TCV3::autoMode);
	connect(ui.SilentModeButton, &QPushButton::clicked, this, &TCV3::silentMode);
	connect(ui.PerformanceModeButton, &QPushButton::clicked, this, &TCV3::performanceMode);

	// 其他按钮
	connect(ui.resetSettingsButton, &QPushButton::clicked, this, &TCV3::resetSettings);
	connect(ui.autoStartButton, &QPushButton::clicked, this, &TCV3::autoStart);
	connect(ui.dataTxDelayButton, &QPushButton::clicked, this, &TCV3::setdataDelay);

	connect(ui.supportButton, &QPushButton::clicked, []() {
		QDesktopServices::openUrl(QUrl("https://ankali-aylina.github.io/2025/05/05/IRSTB/"));
	});

	m_updateLogDialog = new UpdateLogDialog(this);
	connect(ui.updataLogButton, &QPushButton::clicked, m_updateLogDialog, &QDialog::open);
}

TCV3::~TCV3()
{
	if (m_moduleManager)
	{
		m_moduleManager->stopAll();
	}
	delete trayIcon;
	delete m_exitWidgets;
	delete m_updateLogDialog;
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

/// <summary>
/// 预加载图标
/// </summary>
void TCV3::preloadIcons()
{
	// 初始化菜单栏图标缓存
	m_buttonIcons = {
		{ QIcon(":/TCV3/res/icon/home_fill.png"), QIcon(":/TCV3/res/icon/home_line.png") },
		{ QIcon(":/TCV3/res/icon/settings_fill.png"), QIcon(":/TCV3/res/icon/settings_line.png") },
		{ QIcon(":/TCV3/res/icon/about_fill.png"), QIcon(":/TCV3/res/icon/about_line.png") }
	};

	m_cpuNormalIcon = QIcon(":/TCV3/res/icon/cpuTemp.png");
	m_cpuWarningIcon = QIcon(":/TCV3/res/icon/cpuTemp-red.png");
	m_gpuNormalIcon = QIcon(":/TCV3/res/icon/gpuTemp.png");
	m_gpuWarningIcon = QIcon(":/TCV3/res/icon/gpuTemp-red.png");

	m_buttonOnIcon = QIcon(":/TCV3/res/icon/Button_On.png");
	m_buttonOffIcon = QIcon(":/TCV3/res/icon/Button_Off.png");
}

/// <summary>
/// 页面切换动画（淡入淡出）
/// </summary>
void TCV3::animatePageSwitch(int targetIndex)
{
	const int currentIndex = ui.stackedWidget->currentIndex();
	if (currentIndex == targetIndex) return;

	// 先更新按钮图标（立即响应）
	// 注意：图标根据 targetIndex 更新，因为下面会切换页面
	for (int i = 0; i < m_buttons.size(); ++i) {
		m_buttons[i]->setIcon(i == targetIndex ?
			m_buttonIcons[i].active :
			m_buttonIcons[i].inactive);
	}

	auto* fadeOut = new QPropertyAnimation(m_pageOpacityEffect, "opacity", this);
	fadeOut->setDuration(150);
	fadeOut->setStartValue(1.0);
	fadeOut->setEndValue(0.0);
	fadeOut->setEasingCurve(QEasingCurve::InOutCubic);

	connect(fadeOut, &QPropertyAnimation::finished, this, [this, targetIndex]() {
		ui.stackedWidget->setCurrentIndex(targetIndex);

		auto* fadeIn = new QPropertyAnimation(m_pageOpacityEffect, "opacity", this);
		fadeIn->setDuration(200);
		fadeIn->setStartValue(0.0);
		fadeIn->setEndValue(1.0);
		fadeIn->setEasingCurve(QEasingCurve::OutCubic);
		fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
	});

	fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
}

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

void TCV3::onCpuTemperatureUpdated(int temperature)
{
	updateTempDisplay(temperature, m_warningCpuThreshold,
		ui.cpuTempIcon, ui.cpuTempLabel, ui.cpuTempProgressBar,
		m_cpuNormalIcon, m_cpuWarningIcon);
}

void TCV3::onGpuTemperatureUpdated(int temperature)
{
	updateTempDisplay(temperature, m_warningGpuThreshold,
		ui.gpuTempIcon, ui.gpuTempLabel, ui.gpuTempProgressBar,
		m_gpuNormalIcon, m_gpuWarningIcon);
}

void TCV3::updateTempDisplay(int temperature, int warningThreshold,
	QAbstractButton* iconBtn, QLabel* tempLabel, QProgressBar* bar,
	const QIcon& normalIcon, const QIcon& warningIcon)
{
	if (temperature > warningThreshold) {
		iconBtn->setIcon(warningIcon);
		tempLabel->setStyleSheet(QStringLiteral("color: #e04050;"));
	} else {
		iconBtn->setIcon(normalIcon);
		tempLabel->setStyleSheet(QStringLiteral("color: #e8e8e8;"));
	}
	bar->setLayoutDirection(Qt::LeftToRight);
	bar->setValue(100 - temperature);
	bar->setMaximum(100);
	tempLabel->setText(QString::number(temperature) + QStringLiteral("℃"));
}

void TCV3::setFanMode(int activeIdx, bool enableAutoConnection)
{
	// 统一更新按钮图标和启用状态
	for (int i = 0; i < m_fanModeButtons.size(); ++i)
	{
		bool isActive = (i == activeIdx);
		m_fanModeButtons[i]->setIcon(isActive ? m_buttonOnIcon : m_buttonOffIcon);
		m_fanModeButtons[i]->setEnabled(!isActive);
	}

	// 统一管理自动模式连接
	if (enableAutoConnection)
	{
		if (!m_fanControlConnection)
		{
			auto* tcc = m_moduleManager->getModule<TCCore>();
			auto* ble = m_moduleManager->getModule<BLEThread>();
			m_fanControlConnection = connect(tcc, &TCCore::controlDataUpdated,
			                                  ble, &BLEThread::controlFan);
		}
	}
	else
	{
		if (m_fanControlConnection)
		{
			disconnect(m_fanControlConnection);
			m_fanControlConnection = {};
		}
	}
}

void TCV3::autoMode()        { setFanMode(0, true);  emit setFanAutoMode(); }
void TCV3::silentMode()      { setFanMode(1, false); emit setFanSilentMode(); }
void TCV3::performanceMode() { setFanMode(2, false); emit setFanPerformanceMode(); }

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
		// 先释放单实例锁，再启动新实例（避免竞态条件）
		ApplicationBootstrap::releaseLock();
		setAutoStart(false);  // 清除注册表中的开机自启项
		m_config->deleteFile();
		QProcess::startDetached(QApplication::applicationFilePath());
		QApplication::quit(); // 关闭当前程序
	}
}

void TCV3::initConfigFile()
{
	if (!m_config->isFirstRun())
		return;

	// 首次运行：写入所有默认值
	if (!m_config->isInitialized("UI"))
	{
		m_config->initSection("UI", "false");
		m_config->write("UI", "dataTxDelay", "5");
		m_config->initSection("UI", "true");
	}

	// 标记首次运行完成
	m_config->markFirstRunDone();
}

void TCV3::autoStart()
{
	if (!isAutoStartEnabled())
	{
		setAutoStart(true);
	}
	else
	{
		setAutoStart(false);
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
	int uiDelay = m_config->read("UI", "dataTxDelay").toInt();

	if (uiDelay >= kMinDelay && uiDelay <= kMaxDelay)
	{
		ui.dataDelayLabel->setText(QString::number(uiDelay));
	}
	else {
		emit logMessage(QString("超出调节范围！重置为初始值！"), LogManagement::LOG_ERROR);
		m_config->write("UI", "dataTxDelay", "5");
		uiDelay = m_config->read("UI", "dataTxDelay").toInt();
		ui.dataDelayLabel->setText(QString::number(uiDelay));
	}
}

void TCV3::setdataDelay()
{
	int uiDelay = ui.dataDelayLabel->text().toInt();
	int dataDelay = m_config->read("TC", "DataTransmissionDelay").toInt();
	if (uiDelay == dataDelay / 1000)
	{
		return;
	}
	else
	{
		m_config->write("TC", "DataTransmissionDelay", QString::number(uiDelay * 1000));

		emit  setDataTrDelayUpdataFlag(true);

		emit logMessage(QString("设置延时为%1s").arg(uiDelay), LogManagement::LOG_INFO);
	}
}

void TCV3::dataDelayUiUpdate(int data)
{
	if (data >= kMinDelay && data <= kMaxDelay)
	{
		ui.dataDelayLabel->setText(QString::number(data));
	}
	else {
		emit logMessage(QString("超出调节范围！"), LogManagement::LOG_ERROR);
	}
}

void TCV3::addDelay()
{
	int dataDelay = m_config->read("UI", "dataTxDelay").toInt();
	if (dataDelay <= kMaxDelay)
	{
		dataDelay++;
		m_config->write("UI", "dataTxDelay", QString::number(dataDelay));
		dataDelayUiUpdate(dataDelay);
	}
	else
	{
		emit logMessage(QString("超出调节范围！"), LogManagement::LOG_ERROR);
	}
}

void TCV3::minusDelay()
{
	int dataDelay = m_config->read("UI", "dataTxDelay").toInt();
	if (dataDelay >= kMinDelay)
	{
		dataDelay--;
		m_config->write("UI", "dataTxDelay", QString::number(dataDelay));
		dataDelayUiUpdate(dataDelay);
	}
	else
	{
		emit logMessage(QString("超出调节范围！"), LogManagement::LOG_ERROR);
	}
}