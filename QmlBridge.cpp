#include "QmlBridge.h"

#include "AppModuleManager.h"
#include "ApplicationBootstrap.h"
#include "BLEThread.h"
#include "IConfigProvider.h"
#include "IniManagement.h"
#include "TCCore.h"

#include <QAction>
#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QTextStream>
#include <QTimer>
#include <QUrl>

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

// Windows 11 22H2+：系统背景材质（SDK 未定义时用数值兜底，兼容旧 SDK/Win10）
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif
#ifndef DWMSBT_NONE
#define DWMSBT_NONE 1
#define DWMSBT_MAINWINDOW 2
#define DWMSBT_TRANSIENTWINDOW 3
#endif
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWCP_ROUND
#define DWMWCP_DEFAULT 0
#define DWMWCP_DONOTROUND 1
#define DWMWCP_ROUND 2
#define DWMWCP_ROUNDSMALL 3
#endif

QmlBridge::QmlBridge(QObject* parent)
	: QObject(parent)
{
	// --- 日志（全局单例） ---
	connect(this, &QmlBridge::logMessage, &LogManagement::instance(), &LogManagement::logMessage);

	// --- 模块生命周期（与原 TCV3 构造函数一致） ---
	setupModules();
	setupBleConnections();
	setupTrayIcon();

	// --- 从配置加载 UI 初始状态 ---
	{
		QVariant v = m_config->read("TC", "WarningCpu");
		if (v.isValid()) m_warningCpu = v.toInt();
		v = m_config->read("TC", "WarningGpu");
		if (v.isValid()) m_warningGpu = v.toInt();
	}

	// 延时：唯一数据源 TC/DataTransmissionDelay（毫秒），UI 秒换算
	{
		int delayMs = m_config->read("TC", "DataTransmissionDelay").toInt();
		int seconds = delayMs / 1000;
		if (seconds >= kMinDelay && seconds <= kMaxDelay)
			m_delayUiSeconds = seconds;
		else
		{
			m_config->write("TC", "DataTransmissionDelay", QString::number(kDefaultDelay * 1000));
			m_delayUiSeconds = kDefaultDelay;
		}
	}

	m_autoStartEnabled = isAutoStartEnabled();
	m_systemDark = detectSystemDark();

	// 监听系统深浅色切换（注册表轮询，2 秒一次，开销可忽略）
	{
		auto* timer = new QTimer(this);
		timer->setInterval(2000);
		connect(timer, &QTimer::timeout, this, [this]() {
			const bool dark = detectSystemDark();
			if (dark != m_systemDark) {
				m_systemDark = dark;
				emit systemDarkChanged(dark);
			}
		});
		timer->start();
	}

	// 外观：主题模式与背景材质（默认 跟随系统 + Mica）
	{
		QVariant v = m_config->read("App", "ThemeMode");
		if (v.isValid()) m_themeMode = v.toInt();
		if (m_themeMode < 0 || m_themeMode > 2) m_themeMode = 0;
		v = m_config->read("App", "BackdropType");
		if (v.isValid()) m_backdropType = v.toInt();
		if (m_backdropType < 0 || m_backdropType > 2) m_backdropType = 1;
	}
}

QmlBridge::~QmlBridge()
{
	if (m_moduleManager)
		m_moduleManager->stopAll();
}

// ============================================================================
// 模块生命周期
// ============================================================================

void QmlBridge::setupModules()
{
	auto* iniConfig = new IniManagement(this);
	m_config = iniConfig;

	m_moduleManager = new AppModuleManager(this);

	auto* tcc = m_moduleManager->registerModule<TCCore>(m_config);
	auto* ble = m_moduleManager->registerModule<BLEThread>(m_config);

	if (!m_moduleManager->initializeAll())
	{
		emit logMessage(QStringLiteral("模块初始化失败，部分功能可能不可用！"), LogManagement::LOG_ERROR);
	}
	m_moduleManager->startAll();

	// --- 跨模块连接（TCV3 是唯一信号/槽中枢，此处为 QML 版中枢） ---
	// 自动模式：TCCore 控制数据 → BLE 风扇控制
	m_fanControlConnection = connect(tcc, &TCCore::controlDataUpdated,
	                                 ble, &BLEThread::controlFan,
	                                 Qt::QueuedConnection);
	// TCCore 连接状态检测 → BLE 更新
	connect(tcc, &TCCore::updateConnectionStatus, ble, &BLEThread::updateConnectionStatus,
	        Qt::QueuedConnection);

	// --- 温度 → QML 属性 ---
	connect(tcc, &TCCore::cpuTemperatureUpdated, this, [this](int temperature) {
		m_cpuTemp = temperature;
		emit cpuTempChanged(temperature);
	});
	connect(tcc, &TCCore::gpuTemperatureUpdated, this, [this](int temperature) {
		m_gpuTemp = temperature;
		emit gpuTempChanged(temperature);
	});
}

void QmlBridge::setupBleConnections()
{
	auto* ble = m_moduleManager->getModule<BLEThread>();
	if (!ble) return;

	connect(ble, &BLEThread::bleScanStarted, this, [this]() {
		setBleUiState(BleScanning, QStringLiteral("搜索中..."));
	});
	connect(ble, &BLEThread::bleScanTimeout, this, [this]() {
		setBleUiState(BleFailed, QStringLiteral("搜索超时"));
	});
	connect(ble, &BLEThread::connectionInProgress, this, [this]() {
		setBleUiState(BleConnecting, QStringLiteral("连接中..."));
	});
	connect(ble, &BLEThread::connected, this, [this]() {
		setBleUiState(BleConnected, QStringLiteral("连接完成"));
	});
	connect(ble, &BLEThread::connectionFailed, this, [this]() {
		setBleUiState(BleFailed, QStringLiteral("连接失败"));
	});
	connect(ble, &BLEThread::disconnected, this, [this]() {
		setBleUiState(BleDisconnected, QStringLiteral("断开连接"));
	});
}

void QmlBridge::setBleUiState(BleUiState state, const QString& text)
{
	m_bleState = state;
	m_bleStatusText = text;
	emit bleStateChanged();
}

void QmlBridge::setupTrayIcon()
{
	m_trayIcon = new QSystemTrayIcon(this);
	m_trayIcon->setIcon(QIcon(":/TCV3/res/icon/TrayIcon.png"));
	m_trayIcon->setToolTip(QStringLiteral("智能散热小桌板"));

	auto* menu = new QMenu();
	auto* restoreAction = menu->addAction(QStringLiteral("恢复窗口"));
	auto* quitAction = menu->addAction(QStringLiteral("退出程序"));
	connect(restoreAction, &QAction::triggered, this, &QmlBridge::restoreFromTray);
	connect(quitAction, &QAction::triggered, this, &QmlBridge::quitApp);
	connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
		if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger)
			restoreFromTray();
	});

	m_trayIcon->setContextMenu(menu);
	m_trayIcon->show();
}

// ============================================================================
// 风扇模式
// ============================================================================

void QmlBridge::setFanMode(int mode)
{
	auto* tcc = m_moduleManager->getModule<TCCore>();
	auto* ble = m_moduleManager->getModule<BLEThread>();
	if (!tcc || !ble) return;

	if (mode == 0) // 自动：温度驱动调速
	{
		if (!m_fanControlConnection)
		{
			m_fanControlConnection = connect(tcc, &TCCore::controlDataUpdated,
			                                 ble, &BLEThread::controlFan,
			                                 Qt::QueuedConnection);
		}
		ble->autoMode();
	}
	else // 手动模式（静音/全速）：断开自动调速连接
	{
		if (m_fanControlConnection)
		{
			disconnect(m_fanControlConnection);
			m_fanControlConnection = {};
		}
		if (mode == 1) ble->silentMode();
		else           ble->performanceMode();
	}
}

// ============================================================================
// 蓝牙
// ============================================================================

void QmlBridge::reconnectBle()
{
	auto* ble = m_moduleManager->getModule<BLEThread>();
	if (!ble) return;

	setBleUiState(BleConnecting, QStringLiteral("重连中..."));
	ble->reconnectDevice();
}

// ============================================================================
// 延时
// ============================================================================

void QmlBridge::changeDelay(int delta)
{
	const int v = m_delayUiSeconds + delta;
	if (v < kMinDelay || v > kMaxDelay)
	{
		emit logMessage(QStringLiteral("超出调节范围！"), LogManagement::LOG_ERROR);
		return;
	}
	m_delayUiSeconds = v;
	emit delayUiSecondsChanged(v);
}

void QmlBridge::applyDelay()
{
	m_config->write("TC", "DataTransmissionDelay", QString::number(m_delayUiSeconds * 1000));
	if (auto* tcc = m_moduleManager->getModule<TCCore>())
		tcc->setDataTrDelayUpdataFlag(true);
	emit logMessage(QStringLiteral("设置延时为%1s").arg(m_delayUiSeconds), LogManagement::LOG_INFO);
}

// ============================================================================
// 开机自启（直接读写注册表，不写入 config.ini）
// ============================================================================

bool QmlBridge::detectSystemDark()
{
	// 读注册表 AppsUseLightTheme：0=深色 1=浅色。
	// 比 Qt.styleHints.colorScheme 可靠（后者在部分系统/样式下返回 Unknown）
	QSettings settings(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
	                   QSettings::NativeFormat);
	const int light = settings.value(QStringLiteral("AppsUseLightTheme"), 1).toInt();
	return light == 0;
}

bool QmlBridge::isAutoStartEnabled()
{
	QSettings settings(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
	                   QSettings::NativeFormat);
	QString regValue = settings.value(QStringLiteral("TemperatureControlV3")).toString().trimmed();
	if (regValue.startsWith('"') && regValue.endsWith('"'))
		regValue = regValue.mid(1, regValue.length() - 2);

	QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
	return settings.contains(QStringLiteral("TemperatureControlV3")) && (regValue == appPath);
}

void QmlBridge::setAutoStart(bool enable)
{
	QSettings settings(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
	                   QSettings::NativeFormat);
	QString appPath = "\"" + QDir::toNativeSeparators(QCoreApplication::applicationFilePath()) + "\"";
	if (enable)
		settings.setValue(QStringLiteral("TemperatureControlV3"), appPath);
	else
		settings.remove(QStringLiteral("TemperatureControlV3"));
}

void QmlBridge::toggleAutoStart()
{
	setAutoStart(!m_autoStartEnabled);
	m_autoStartEnabled = isAutoStartEnabled();
	emit autoStartEnabledChanged(m_autoStartEnabled);
}

// ============================================================================
// 重置 / 关于 / 退出
// ============================================================================

void QmlBridge::resetSettings()
{
	QMessageBox::StandardButton reply = QMessageBox::question(
		nullptr, QStringLiteral("重启确认"),
		QStringLiteral("删除配置会重启应用，是否继续？"),
		QMessageBox::Yes | QMessageBox::No);

	if (reply == QMessageBox::Yes)
	{
		// 先释放单实例锁，再启动新实例（避免竞态条件）
		ApplicationBootstrap::releaseLock();
		setAutoStart(false);
		m_config->deleteFile();
		QProcess::startDetached(QApplication::applicationFilePath());
		QApplication::quit();
	}
}

// ============================================================================
// 主题模式与背景材质（WinUI Mica/Acrylic 风格）
// ============================================================================

void QmlBridge::setThemeMode(int mode)
{
	if (mode < 0 || mode > 2) return;
	if (mode == m_themeMode) return;
	m_themeMode = mode;
	m_config->write("App", "ThemeMode", QString::number(mode));
	emit themeModeChanged(mode);
	// QML 侧根据 themeMode + 系统配色方案计算 isDark，再回调 applyDarkMode 更新 DWM
}

void QmlBridge::setBackdropType(int type)
{
	if (type < 0 || type > 2) return;
	if (type == m_backdropType) return;
	m_backdropType = type;
	m_config->write("App", "BackdropType", QString::number(type));
	emit backdropTypeChanged(type);
	applyDwmBackdrop();
}

void QmlBridge::applyWindowBackdrop(WId hwnd)
{
	m_hwnd = hwnd;
	applyDwmBackdrop();
}

void QmlBridge::applyDwmBackdrop()
{
	if (!m_hwnd) return;

	// 材质映射：0=无 1=Mica(DWMSBT_MAINWINDOW) 2=Acrylic(DWMSBT_TRANSIENTWINDOW)
	int type = DWMSBT_NONE;
	switch (m_backdropType) {
	case 1: type = DWMSBT_MAINWINDOW; break;      // Mica
	case 2: type = DWMSBT_TRANSIENTWINDOW; break; // Acrylic
	default: type = DWMSBT_NONE; break;
	}
	// Win10 不支持时 DwmSetWindowAttribute 返回失败，窗口退化为半透明纯色背景
	DwmSetWindowAttribute(reinterpret_cast<HWND>(m_hwnd), DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(type));

	// Win11：让窗口本身圆角化，透明窗口的四角由系统裁剪，
	// 不再透出 Mica/Acrylic 材质的"半透明直角"
	const int cornerPref = DWMWCP_ROUND;
	DwmSetWindowAttribute(reinterpret_cast<HWND>(m_hwnd), DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPref, sizeof(cornerPref));
}

void QmlBridge::applyDarkMode(bool dark)
{
	if (!m_hwnd) return;
	const DWORD mode = dark ? 1 : 0;
	DwmSetWindowAttribute(reinterpret_cast<HWND>(m_hwnd), DWMWA_USE_IMMERSIVE_DARK_MODE, &mode, sizeof(mode));
}

void QmlBridge::openSupportUrl()
{
	QDesktopServices::openUrl(QUrl(QStringLiteral("https://ankali-aylina.github.io/2025/05/05/IRSTB/")));
}

QString QmlBridge::readUpdateLog()
{
	QFile file(QStringLiteral(":/TCV3/res/updatalog.md"));
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return QStringLiteral("无法打开更新日志");
	QTextStream stream(&file);
	return stream.readAll();
}

void QmlBridge::quitApp()
{
	QApplication::quit();
}

void QmlBridge::restoreFromTray()
{
	emit restoreRequested();
}
